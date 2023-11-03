#include "server.h"

MongooseHttpServer server;

// Variable to hold the last modification datetime
char last_modified[50];

CircularBuffer<WebsocketRequest*, YB_RECEIVE_BUFFER_COUNT> wsRequests;

MongooseHttpWebSocketConnection * authenticatedConnections[YB_CLIENT_LIMIT];

const char *server_pem = 
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDDjCCAfagAwIBAgIBBDANBgkqhkiG9w0BAQsFADA/MRkwFwYDVQQDDBB0ZXN0\r\n"
"LmNlc2FudGEuY29tMRAwDgYDVQQKDAdDZXNhbnRhMRAwDgYDVQQLDAd0ZXN0aW5n\r\n"
"MB4XDTE2MTExMzEzMTgwMVoXDTI2MDgxMzEzMTgwMVowFDESMBAGA1UEAwwJbG9j\r\n"
"YWxob3N0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAro8CW1X0xaGm\r\n"
"GkDaMxKbXWA5Lw+seA61tioGrSIQzuqLYeJoFnwVgF0jB5PTj+3EiGMBcA/mh73V\r\n"
"AthTFmJBxj+agIp7/cvUBpgfLClmSYL2fZi6Fodz+f9mcry3XRw7O6vlamtWfTX8\r\n"
"TAmMSR6PXVBHLgjs5pDOFFmrNAsM5sLYU1/1MFvE2Z9InTI5G437IE1WchRSbpYd\r\n"
"HchC39XzpDGoInZB1a3OhcHm+xUtLpMJ0G0oE5VFEynZreZoEIY4JxspQ7LPsay9\r\n"
"fx3Tlk09gEMQgVCeCNiQwUxZdtLau2x61LNcdZCKN7FbFLJszv1U2uguELsTmi7E\r\n"
"6pHrTziosQIDAQABo0AwPjAJBgNVHRMEAjAAMAsGA1UdDwQEAwIDqDATBgNVHSUE\r\n"
"DDAKBggrBgEFBQcDATAPBgNVHREECDAGhwR/AAABMA0GCSqGSIb3DQEBCwUAA4IB\r\n"
"AQBUw0hbTcT6crzODO4QAXU7z4Xxn0LkxbXEsoThG1QCVgMc4Bhpx8gyz5CLyHYz\r\n"
"AiJOBFEeV0XEqoGTNMMFelR3Q5Tg9y1TYO3qwwAWxe6/brVzpts6NiG1uEMBnBFg\r\n"
"oN1x3I9x4NpOxU5MU1dlIxvKs5HQCoNJ8D0SqOX9BV/pZqwEgiCbuWDWQAlxkFpn\r\n"
"iLonlkVI5hTuybCSBsa9FEI9M6JJn9LZmlH90FYHeS4t6P8eOJCeekHL0jUG4Iae\r\n"
"DMP12h8Sd0yxIKmmZ+Q/p/D/BkuHf5Idv3hgyLkZ4mNznjK49wHaYM+BgBoL3Zeg\r\n"
"gJ2sWjUlokrbHswSBLLbUJIF\r\n"
"-----END CERTIFICATE-----\r\n";

const char *server_key = 
"-----BEGIN PRIVATE KEY-----\r\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCujwJbVfTFoaYa\r\n"
"QNozEptdYDkvD6x4DrW2KgatIhDO6oth4mgWfBWAXSMHk9OP7cSIYwFwD+aHvdUC\r\n"
"2FMWYkHGP5qAinv9y9QGmB8sKWZJgvZ9mLoWh3P5/2ZyvLddHDs7q+Vqa1Z9NfxM\r\n"
"CYxJHo9dUEcuCOzmkM4UWas0CwzmwthTX/UwW8TZn0idMjkbjfsgTVZyFFJulh0d\r\n"
"yELf1fOkMagidkHVrc6Fweb7FS0ukwnQbSgTlUUTKdmt5mgQhjgnGylDss+xrL1/\r\n"
"HdOWTT2AQxCBUJ4I2JDBTFl20tq7bHrUs1x1kIo3sVsUsmzO/VTa6C4QuxOaLsTq\r\n"
"ketPOKixAgMBAAECggEAI+uNwpnHirue4Jwjyoqzqd1ZJxQEm5f7UIcJZKsz5kBh\r\n"
"ej0KykWybv27bZ2/1UhKPv6QlyzOdXRc1v8I6fxCKLeB5Z2Zsjo1YT4AfCfwwoPO\r\n"
"kT3SXTx2YyVpQYcP/HsIvVi8FtALtixbxJHaall9iugwHYr8pN17arihAE6d0wZC\r\n"
"JXtXRjUWwjKzXP8FoH4KhyadhHbDwIbbJe3cyLfdvp54Gr0YHha0JcOxYgDYNya4\r\n"
"OKxlCluI+hPF31iNzOmFLQVrdYynyPcR6vY5XOiANKE2iNbqCzRb54CvW9WMqObX\r\n"
"RD9t3DMOxGsbVNIwyzZndWy13HoQMGnrHfnGak9ueQKBgQDiVtOqYfLnUnTxvJ/b\r\n"
"qlQZr2ZmsYPZztxlP+DSqZGPD+WtGSo9+rozWfzjTv3KGIDLvf+GFVmjVHwlLQfd\r\n"
"u7eTemWHFc4HK68wruzPO/FdyVpQ4w9v3Usg+ll4a/PDEId0fDMjAr6kk4LC6t8y\r\n"
"9fJR0HjOz57jVnlrDt3v50G8BwKBgQDFbw+jRiUxXnBbDyXZLi+I4iGBGdC+CbaJ\r\n"
"CmsM6/TsOFc+GRsPwQF1gCGqdaURw76noIVKZJOSc8I+yiwU6izyh/xaju5JiWQd\r\n"
"kwbU1j4DE6GnxmT3ARmB7VvCxjaEZEAtICWs1QTKRz7PcTV8yr7Ng1A3VIy+NSpo\r\n"
"LFMMmk83hwKBgQDVCEwpLg/mUeHoNVVw95w4oLKNLb+gHeerFLiTDy8FrDzM88ai\r\n"
"l37yHly7xflxYia3nZkHpsi7xiUjCINC3BApKyasQoWskh1OgRY653yCfaYYQ96f\r\n"
"t3WjEH9trI2+p6wWo1+uMEMnu/9zXoW9/WeaQdGzNg+igh29+jxCNTPVuQKBgGV4\r\n"
"CN9vI5pV4QTLqjYOSJvfLDz/mYqxz0BrPE1tz3jAFAZ0PLZCCY/sBGFpCScyJQBd\r\n"
"vWNYgYeZOtGuci1llSgov4eDQfBFTlDsyWwFl+VY55IkoqtXw1ZFOQ3HdSlhpKIM\r\n"
"jZBgApA7QYq3sjeqs5lHzahCKftvs5XKgfxOKjxtAoGBALdnYe6xkDvGLvI51Yr+\r\n"
"Dy0TNcB5W84SxUKvM7DVEomy1QPB57ZpyQaoBq7adOz0pWJXfp7qo4950ZOhBGH1\r\n"
"hKbZ6c4ggwVJy2j49EgMok5NGCKvPAtabbR6H8Mz8DW9aXURxhWJvij+Qw1fWK4b\r\n"
"7G/qUI9iE5iUU7MkIcLIbTf/\r\n"
"-----END PRIVATE KEY-----\r\n";

void server_setup()
{
  // Populate the last modification date based on build datetime
  sprintf(last_modified, "%s %s GMT", __DATE__, __TIME__);

  Mongoose.begin();

  #ifdef YB_USE_SSL
    if(false == server.begin(443, server_pem, server_key)) {
      Serial.print("Failed to start server");
      return;
    }
  #else
    server.begin(80);
  #endif

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
  //Special case for messages during OTA as the update is blocking and will cause
  //all of our sockets to time out and won't let us have a nice little display bar.
  // if (doOTAUpdate)
  // {
  //   char jsonBuffer[YB_MAX_JSON_LENGTH];
  //   //StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
  //   DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  //   StaticJsonDocument<1024> input;
  //   DeserializationError err = deserializeJson(input, data);

  //   String mycmd = input["cmd"] | "???";

  //   //was there a problem, officer?
  //   if (err)
  //   {
  //     char error[64];
  //     sprintf(error, "deserializeJson() failed with code %s", err.c_str());
  //     generateErrorJSON(output, error);
  //   }
  //   else
  //     handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, 0);
  //     //handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, client->id());

  //   //empty messages are valid, so don't send a response
  //   if (output.size())
  //   {
  //     serializeJson(output, jsonBuffer);

  //     // //only send if we're empty.  Ignore it otherwise.
  //     // if (ws.availableForWrite(client->id()))
  //     //   ws.text(client->id(), jsonBuffer);
  //     // else
  //     //   Serial.println("[socket] client full");
  //   }
  // }
  // else
  // {
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
  // }
}

void handleWebsocketMessageLoop(WebsocketRequest* request)
{
  char jsonBuffer[YB_MAX_JSON_LENGTH];
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  StaticJsonDocument<1024> input;
  DeserializationError err = deserializeJson(input, request->buffer);

  //was there a problem, officer?
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