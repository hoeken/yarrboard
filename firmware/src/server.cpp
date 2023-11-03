#include "server.h"

MongooseHttpServer server;

// Variable to hold the last modification datetime
char last_modified[50];

CircularBuffer<WebsocketRequest*, YB_RECEIVE_BUFFER_COUNT> wsRequests;

MongooseHttpWebSocketConnection * authenticatedConnections[YB_CLIENT_LIMIT];

String server_pem;
String server_key;

void server_setup()
{
  // Populate the last modification date based on build datetime
  sprintf(last_modified, "%s %s GMT", __DATE__, __TIME__);

  //do we want https?
  if (preferences.isKey("appEnableHttps"))
    app_enable_https = preferences.getBool("appEnableHttps");

  //look up our keys?
  if (app_enable_https)
  {
    File fp = LittleFS.open("/server.pem");
    if (fp)
      server_pem = fp.readString();
    else
    {
      Serial.println("server.pem not found, SSL not available");
      app_enable_https = false;
    }
    fp.close();

    File fp2 = LittleFS.open("/server.key");
    if (fp2)
      server_key = fp2.readString();
    else
    {
      Serial.println("server.key not found, SSL not available");
      app_enable_https = false;
    }
    fp2.close();
  }

  Mongoose.begin();

  //do we want secure or not?
  if (app_enable_https)
  {
    if(false == server.begin(443, server_pem.c_str(), server_key.c_str())) {
      Serial.print("Failed to start HTTPS server");
      return;
  }
  else
  {
    server.begin(80);
  }

  server.on("/$", HTTP_GET, [](MongooseHttpServerRequest *request) {
    //Check if the client already has the same version and respond with a 304 (Not modified)
    if (request->headers("If-Modified-Since").equals(last_modified)) {
        request->send(304);
    } else {
        MongooseHttpServerResponseBasic *response = request->beginResponse();
        response->setCode(200);
        response->setContentType("text/html");

        // Tell the browswer the contemnt is Gzipped
        response->addHeader("Content-Encoding", "gzip");

        // And set the last-modified datetime so we can check if we need to send it again next time or not
        response->addHeader("Last-Modified", last_modified);

        //add our actual content
        response->setContent(index_html_gz, index_html_gz_len);

        request->send(response);
   }
  });

  // Test the stream response class
  server.on("/ws$")->
    onConnect([](MongooseHttpWebSocketConnection *connection) {
      Serial.println("[socket] new connection");
    })->
    onClose([](MongooseHttpServerRequest *c) {
      MongooseHttpWebSocketConnection *connection = static_cast<MongooseHttpWebSocketConnection *>(c);
      Serial.println("[socket] connection closed");

      //stop tracking the connection
      if (require_login)
        for (byte i=0; i<YB_CLIENT_LIMIT; i++)
          if (authenticatedConnections[i] == connection)
            authenticatedConnections[i] = NULL;
    })->
    onFrame([](MongooseHttpWebSocketConnection *connection, int flags, uint8_t *data, size_t len) {
      handleWebSocketMessage(connection, data, len);
    });

  //our main api connection
  server.on("/api/endpoint$", HTTP_GET, [](MongooseHttpServerRequest *request)
  {
    StaticJsonDocument<1024> json;
    String body = request->body();
    DeserializationError err = deserializeJson(json, body);

    handleWebServerRequest(json, request);
  });

  //send config json
  server.on("/api/config$", HTTP_GET, [](MongooseHttpServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_config";

    handleWebServerRequest(json, request);
  });

  //send stats json
  server.on("/api/stats$", HTTP_GET, [](MongooseHttpServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_stats";

    handleWebServerRequest(json, request);
  });

  //send update json
  server.on("/api/update$", HTTP_GET, [](MongooseHttpServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_update";

    handleWebServerRequest(json, request);
  });

  //downloadable coredump file
  server.on("/coredump.txt$", HTTP_GET, [](MongooseHttpServerRequest *request)
  {
    //delete the coredump here, but not from littlefs
		deleteCoreDump();

    //dont bug the client anymore
    has_coredump = false;

    MongooseHttpServerResponseBasic *response = request->beginResponse();
    response->setContentType("text/plain");

    if (LittleFS.exists("/coredump.txt"))
    {
      response->setCode(200);

      //this feels a little bit hacky...
      File fp = LittleFS.open("/coredump.txt");
      String data = fp.readString();
      response->setContent(data.c_str());
    }
    else
    {
      response->setCode(404);
      response->setContent("Coredump not found.");
    }

    request->send(response);
  });

  //a 404 is nice
  server.onNotFound([](MongooseHttpServerRequest *request)
  {
    request->send(404, "text/plain", "Not found");
  });
}

void server_loop()
{
  Mongoose.poll(0);

  //process our websockets outside the callback.
  if (!wsRequests.isEmpty())
    handleWebsocketMessageLoop(wsRequests.shift());
}

void sendToAllWebsockets(const char * jsonString)
{
  //send the message to all authenticated clients.
  if (require_login)
  {
    for (byte i=0; i<YB_CLIENT_LIMIT; i++)
      if (authenticatedConnections[i] != NULL)
          authenticatedConnections[i]->send(jsonString);
        else
          Serial.println("[socket] client queue full");
  }
  //nope, just sent it to all.
  else
    server.sendAll(jsonString);
}

bool logClientIn(MongooseHttpWebSocketConnection *connection)
{
  //did we not find a spot?
  if (!addClientToAuthList(connection))
  {
    //TODO: is there a way to close a connection?
    //TODO: we need to test the max connections, mongoose may just handle it for us.
    // AsyncWebSocketClient* client = ws.client(client_id);
    // client->close();

    return false;
  }

  return true;
}

void handleWebServerRequest(JsonVariant input, MongooseHttpServerRequest *request)
{
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  if (request->hasParam("user"))
    input["user"] = request->getParam("user");
  if (request->hasParam("pass"))
    input["pass"] = request->getParam("pass");

  if (app_enable_api)
    handleReceivedJSON(input, output, YBP_MODE_HTTP);
  else
    generateErrorJSON(output, "Web API is disabled.");      

  //we can have empty messages
  if (output.size())
  {
    MongooseHttpServerResponseStream *response = request->beginResponseStream();
    response->setContentType("application/json");
    serializeJson(output.as<JsonObject>(), *response);
    request->send(response);
  }
  //give them valid json at least
  else
    request->send(200, "application/json", "{}");
}

void handleWebSocketMessage(MongooseHttpWebSocketConnection *connection, uint8_t *data, size_t len)
{
  if (!wsRequests.isFull())
  {
    WebsocketRequest* wr = new WebsocketRequest;
    wr->client = connection;
    strlcpy(wr->buffer, (char*)data, YB_RECEIVE_BUFFER_LENGTH); 
    wsRequests.push(wr);
  }

  //start throttling a little bit early so we don't miss anything
  if (wsRequests.capacity <= YB_RECEIVE_BUFFER_COUNT/2)
  {
    StaticJsonDocument<128> output;
    String jsonBuffer;
    generateErrorJSON(output, "Websocket busy, throttle connection.");
    serializeJson(output, jsonBuffer);

    connection->send(jsonBuffer);
  }
}

void handleWebsocketMessageLoop(WebsocketRequest* request)
{
  char jsonBuffer[YB_MAX_JSON_LENGTH];
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);
  DynamicJsonDocument input(1024);

  //was there a problem, officer?
  DeserializationError err = deserializeJson(input, request->buffer);
  if (err)
  {
    char error[64];
    sprintf(error, "deserializeJson() failed with code %s", err.c_str());
    generateErrorJSON(output, error);
  }
  else
    handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, request->client);

  //empty messages are valid, so don't send a response
  if (output.size())
  {
    serializeJson(output, jsonBuffer);
    request->client->send(jsonBuffer);
  }

  delete request;
}

bool isLoggedIn(JsonVariantConst input, byte mode, MongooseHttpWebSocketConnection *connection)
{
  //also only if enabled
  if (!require_login)
    return true;

  //login only required for websockets.
  if (mode == YBP_MODE_WEBSOCKET)
    return isWebsocketClientLoggedIn(input, connection);
  else if (mode == YBP_MODE_HTTP)
    return isApiClientLoggedIn(input);
  else if (mode == YBP_MODE_SERIAL)
    return isSerialClientLoggedIn(input);
  else
    return false;
}

bool isWebsocketClientLoggedIn(JsonVariantConst doc, MongooseHttpWebSocketConnection *connection)
{
  //are they in our auth array?
  for (byte i=0; i<YB_CLIENT_LIMIT; i++)
    if (authenticatedConnections[i] == connection)
      return true;

  //okay check for passed-in credentials
  return isApiClientLoggedIn(doc);
}

bool isApiClientLoggedIn(JsonVariantConst doc)
{
  if (!doc.containsKey("user"))
    return false;
  if (!doc.containsKey("pass"))
    return false;

  //init
  char myuser[YB_USERNAME_LENGTH];
  char mypass[YB_PASSWORD_LENGTH];
  strlcpy(myuser, doc["user"] | "", sizeof(myuser));
  strlcpy(mypass, doc["pass"] | "", sizeof(myuser));

  //morpheus... i'm in.
  if (!strcmp(app_user, myuser) && !strcmp(app_pass, mypass))
    return true;

  //default to fail then.
  return false;  
}

bool isSerialClientLoggedIn(JsonVariantConst doc)
{
  if (is_serial_authenticated)
    return true;
  else
    return isApiClientLoggedIn(doc);
}

bool addClientToAuthList(MongooseHttpWebSocketConnection *connection)
{
  byte i;
  for (i=0; i<YB_CLIENT_LIMIT; i++)
  {
    //did we find an empty slot?
    if (authenticatedConnections[i] == NULL)
    {
      authenticatedConnections[i] = connection;
      break;
    }

    //are we already authenticated?
    if (authenticatedConnections[i] == connection)
      break;
  }

  //did we not find a spot?
  if (i == YB_CLIENT_LIMIT)
    return false;
  else
   return true;
}