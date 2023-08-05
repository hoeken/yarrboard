#include "server.h"

//username / password for websocket authentication
char app_user[YB_USERNAME_LENGTH] = "admin";
char app_pass[YB_PASSWORD_LENGTH] = "admin";
bool require_login = true;
bool app_enable_api = true;
bool app_enable_serial = false;
bool is_serial_authenticated = false;

//keep track of our authenticated clients
const byte clientLimit = 8;
uint32_t authenticatedClientIDs[clientLimit];

AsyncWebSocket ws("/ws");
AsyncWebServer server(80);

void server_setup()
{
  //look up our board name
  if (preferences.isKey("boardName"))
    strlcpy(board_name, preferences.getString("boardName").c_str(), sizeof(board_name));

  //look up our username/password
  if (preferences.isKey("app_user"))
    strlcpy(app_user, preferences.getString("app_user").c_str(), sizeof(app_user));
  if (preferences.isKey("app_pass"))
    strlcpy(app_pass, preferences.getString("app_pass").c_str(), sizeof(app_pass));
  if (preferences.isKey("require_login"))
    require_login = preferences.getBool("require_login");
  if (preferences.isKey("appEnableApi"))
    app_enable_api = preferences.getBool("appEnableApi");
  if (preferences.isKey("appEnableSerial"))
    app_enable_serial = preferences.getBool("appEnableSerial");

  //config for our websocket server
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/endpoint", [](AsyncWebServerRequest *request, JsonVariant &json)
  {
    char jsonBuffer[MAX_JSON_LENGTH];

    if (app_enable_api)
    {
      JsonObject doc = json.as<JsonObject>();
      handleReceivedJSON(doc, jsonBuffer, YBP_MODE_HTTP, 0);
    }
    else
      generateErrorJSON(jsonBuffer, "Web API is disabled.");      

    request->send(200, "application/json", jsonBuffer);
  });
  server.addHandler(handler);

  //send config json
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    char jsonBuffer[MAX_JSON_LENGTH];

    if (app_enable_api)
    {
      StaticJsonDocument<256> json;
      json["cmd"] = "get_config";

      if (request->hasParam("user"))
        json["user"] = request->getParam("user")->value();
      if (request->hasParam("pass"))
        json["pass"] = request->getParam("pass")->value();

      JsonObject doc = json.as<JsonObject>();
      handleReceivedJSON(doc, jsonBuffer, YBP_MODE_HTTP, 0);
    }
    else
      generateErrorJSON(jsonBuffer, "Web API is disabled.");      

    request->send(200, "application/json", jsonBuffer);
  });

  //send stats json
  server.on("/api/stats", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    char jsonBuffer[MAX_JSON_LENGTH];

    if (app_enable_api)
    {
      StaticJsonDocument<256> json;
      json["cmd"] = "get_stats";

      if (request->hasParam("user"))
        json["user"] = request->getParam("user")->value();
      if (request->hasParam("pass"))
        json["pass"] = request->getParam("pass")->value();

      JsonObject doc = json.as<JsonObject>();
      handleReceivedJSON(doc, jsonBuffer, YBP_MODE_HTTP, 0);
    }
    else
      generateErrorJSON(jsonBuffer, "Web API is disabled.");      
    
    request->send(200, "application/json", jsonBuffer);
  });

  //send update json
  server.on("/api/update", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    char jsonBuffer[MAX_JSON_LENGTH];

    if (app_enable_api)
    {
      StaticJsonDocument<256> json;
      json["cmd"] = "get_update";

      if (request->hasParam("user"))
        json["user"] = request->getParam("user")->value();
      if (request->hasParam("pass"))
        json["pass"] = request->getParam("pass")->value();

      JsonObject doc = json.as<JsonObject>();
      handleReceivedJSON(doc, jsonBuffer, YBP_MODE_HTTP, 0);
    }
    else
      generateErrorJSON(jsonBuffer, "Web API is disabled.");      

    request->send(200, "application/json", jsonBuffer);
  });

  //we are only serving static files - big cache
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=2592000");

  server.begin();
}

void server_loop()
{
  //sometimes websocket clients die badly.
  ws.cleanupClients();
}

void sendToAllWebsockets(const char * jsonString)
{
  //send the message to all authenticated clients.
  if (require_login)
  {
    for (byte i=0; i<clientLimit; i++)
      if (authenticatedClientIDs[i])
        if (ws.availableForWrite(authenticatedClientIDs[i]))
          ws.text(authenticatedClientIDs[i], jsonString);
  }
  //nope, just sent it to all.
  else {
    if (ws.availableForWriteAll())
      ws.textAll(jsonString);
    else
      Serial.println("[socket] queue full");
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
        for (byte i=0; i<clientLimit; i++)
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
    char jsonBuffer[MAX_JSON_LENGTH];

    StaticJsonDocument<1024> json;
    DeserializationError err = deserializeJson(json, (char *)data);
    JsonObject doc = json.as<JsonObject>();

    //was there a problem, officer?
    if (err)
    {
      char error[64];
      sprintf(error, "deserializeJson() failed with code %s", err.c_str());
      generateErrorJSON(jsonBuffer, error);
    }
    else
      handleReceivedJSON(doc, jsonBuffer, YBP_MODE_WEBSOCKET, client->id());

    if (client->canSend())
      ws.text(client->id(), jsonBuffer);
  }
}

bool isWebsocketClientLoggedIn(const JsonObject& doc, uint32_t client_id)
{
  //are they in our auth array?
  for (byte i=0; i<clientLimit; i++)
    if (authenticatedClientIDs[i] == client_id)
      return true;

  //okay check for passed-in credentials
  return isApiClientLoggedIn(doc);
}

bool isApiClientLoggedIn(const JsonObject& doc)
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

bool isSerialClientLoggedIn(const JsonObject& doc)
{
  if (is_serial_authenticated)
    return true;
  else
    return isApiClientLoggedIn(doc);
}

bool logClientIn(uint32_t client_id)
{
  byte i;
  for (i=0; i<clientLimit; i++)
  {
    //did we find an empty slot?
    if (authenticatedClientIDs[i] == 0)
    {
      authenticatedClientIDs[i] = client_id;
      break;
    }

    //are we already authenticated?
    if (authenticatedClientIDs[i] == client_id)
      break;
  }

  //did we not find a spot?
  if (i == clientLimit)
  {
    AsyncWebSocketClient* client = ws.client(client_id);
    client->close();

    return false;
  }

  return true;
}