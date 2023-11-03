#include "server.h"

// Variable to hold the last modification datetime
char last_modified[50];

CircularBuffer<WebsocketRequest*, YB_RECEIVE_BUFFER_COUNT> wsRequests;

AsyncWebSocket ws("/ws");
AsyncWebServer server(80);

void server_setup()
{
  // Populate the last modification date based on build datetime
  sprintf(last_modified, "%s %s GMT", __DATE__, __TIME__);

  //config for our websocket server
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  //main API endpoint
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/endpoint", [](AsyncWebServerRequest *request, JsonVariant &json)
  {
    handleWebServerRequest(json, request);
  });
  server.addHandler(handler);

  //send config json
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_config";

    handleWebServerRequest(json, request);
  });

  //send stats json
  server.on("/api/stats", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_stats";

    handleWebServerRequest(json, request);
  });

  //send update json
  server.on("/api/update", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    StaticJsonDocument<256> json;
    json["cmd"] = "get_update";

    handleWebServerRequest(json, request);
  });

  //our home page
  server.rewrite("/", "/index.html");
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    // Check if the client already has the same version and respond with a 304 (Not modified)
    if (request->header("If-Modified-Since").equals(last_modified)) {
        request->send(304);
    } else {
        // Dump the byte array in PROGMEM with a 200 HTTP code (OK)
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);

        // Tell the browswer the contemnt is Gzipped
        response->addHeader("Content-Encoding", "gzip");

        // And set the last-modified datetime so we can check if we need to send it again next time or not
        response->addHeader("Last-Modified", last_modified);

        request->send(response);
    }
  });

  //downloadable coredump file
  server.on("/coredump.txt", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    //delete the coredump here, but not from littlefs
		deleteCoreDump();

    //dont bug the client anymore
    has_coredump = false;

    //send the file
    request->send(LittleFS, "/coredump.txt", "text/plain");
  });

  //a 404 is nice
  server.onNotFound([](AsyncWebServerRequest *request){ request->send(404, "text/plain", "Not found"); });

  server.begin();
}

void server_loop()
{
  //sometimes websocket clients die badly.
  ws.cleanupClients();

  //process our websockets outside the callback.
  if (!wsRequests.isEmpty())
    handleWebsocketMessageLoop(wsRequests.shift());
}

void handleWebServerRequest(JsonVariant input, AsyncWebServerRequest *request)
{
  //StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  if (request->hasParam("user"))
    input["user"] = request->getParam("user")->value();
  if (request->hasParam("pass"))
    input["pass"] = request->getParam("pass")->value();

  if (app_enable_api)
    handleReceivedJSON(input, output, YBP_MODE_HTTP, 0);
  else
    generateErrorJSON(output, "Web API is disabled.");      

  //we can have empty messages
  if (output.size())
  {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(output.as<JsonObject>(), *response);
    request->send(response);
  }
  //give them valid json at least
  else
    request->send(200, "application/json", "{}");
}

void sendToAllWebsockets(const char * jsonString)
{
  //send the message to all authenticated clients.
  if (require_login)
  {
    for (byte i=0; i<YB_CLIENT_LIMIT; i++)
      if (authenticatedClientIDs[i])
        if (ws.availableForWrite(authenticatedClientIDs[i]))
          ws.text(authenticatedClientIDs[i], jsonString);
        else
          Serial.println("[socket] client queue full");
  }
  //nope, just sent it to all.
  else {
    ws.textAll(jsonString);
    // if (ws.availableForWriteAll())
    //   ws.textAll(jsonString);
    // else
    //   Serial.println("[socket] outbound queue full");
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[socket] #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("[socket] #%u disconnected\n", client->id());

      //clear this guy from our authenticated list.
      if (require_login)
        for (byte i=0; i<YB_CLIENT_LIMIT; i++)
          if (authenticatedClientIDs[i] == client->id())
            authenticatedClientIDs[i] = 0;

      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len, client);
      break;
    case WS_EVT_PONG:
      Serial.printf("[socket] #%u pong", client->id());
      break;
    case WS_EVT_ERROR:
      Serial.printf("[socket] #%u error", client->id());
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    //Special case for messages during OTA as the update is blocking and will cause
    //all of our sockets to time out and won't let us have a nice little display bar.
    if (doOTAUpdate)
    {
      char jsonBuffer[YB_MAX_JSON_LENGTH];
      //StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
      DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

      StaticJsonDocument<1024> input;
      DeserializationError err = deserializeJson(input, data);

      String mycmd = input["cmd"] | "???";

      //was there a problem, officer?
      if (err)
      {
        char error[64];
        sprintf(error, "deserializeJson() failed with code %s", err.c_str());
        generateErrorJSON(output, error);
      }
      else
        handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, client->id());

      //empty messages are valid, so don't send a response
      if (output.size())
      {
        serializeJson(output, jsonBuffer);

        //only send if we're empty.  Ignore it otherwise.
        if (ws.availableForWrite(client->id()))
          ws.text(client->id(), jsonBuffer);
        else
          Serial.println("[socket] client full");
      }
    }
    else
    {
      if (!wsRequests.isFull())
      {
        WebsocketRequest* wr = new WebsocketRequest;
        wr->client_id = client->id();
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

        client->text(jsonBuffer);
      }
    }
  }
}

void closeClientConnection(AsyncWebSocketClient *client)
{
  //you lose your slots
  //for (byte i=0; i<YB_RECEIVE_BUFFER_COUNT; i++)
  //  if (receiveClientId[i] == client->id())
  //    websocketRequestReady[i] = false;

  //no more auth for you
  for (byte i=0; i<YB_CLIENT_LIMIT; i++)
    if (authenticatedClientIDs[i] == client->id())
      authenticatedClientIDs[i] = 0;

  //goodbye
  client->close();
}

void handleWebsocketMessageLoop(WebsocketRequest* request)
{
  unsigned long start = micros();
  unsigned long t1, t2, t3, t4 = 0;

  char jsonBuffer[YB_MAX_JSON_LENGTH];
  //StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  StaticJsonDocument<1024> input;
  DeserializationError err = deserializeJson(input, request->buffer);

  String mycmd = input["cmd"] | "???";

  t1 = micros();

  //was there a problem, officer?
  if (err)
  {
    char error[64];
    sprintf(error, "deserializeJson() failed with code %s", err.c_str());
    generateErrorJSON(output, error);
  }
  else
    handleReceivedJSON(input, output, YBP_MODE_WEBSOCKET, request->client_id);

  t2 = micros();

  //empty messages are valid, so don't send a response
  if (output.size())
  {
    serializeJson(output, jsonBuffer);
    t3 = micros();

    //only send if we're empty.  Ignore it otherwise.
    if (ws.availableForWrite(request->client_id))
      ws.text(request->client_id, jsonBuffer);
    else
      Serial.println("[socket] client full");
  }
  else
    t3 = micros();

  delete request;

  t4 = micros();
  unsigned long finish = micros();

  if (finish-start > 15000)
  {
    Serial.println(mycmd);
    Serial.printf("deserialize: %dus\n", t1-start); 
    Serial.printf("handle: %dus\n", t2-t1); 
    Serial.printf("serialize: %dus\n", t3-t2); 
    Serial.printf("transmit: %dus\n", t4-t3); 
    Serial.printf("total: %dus\n", finish-start);
    Serial.println();
  }
}

bool logClientIn(uint32_t client_id)
{
  //did we not find a spot?
  if (!addClientToAuthList(client_id))
  {
    AsyncWebSocketClient* client = ws.client(client_id);
    client->close();

    return false;
  }

  return true;
}