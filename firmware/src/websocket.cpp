#include "websocket.h"
#include "prefs.h"
#include "wifi.h"
#include "ota.h"
#include "adc.h"

String board_name = "Yarrboard";

//username / password for websocket authentication
String app_user = "admin";
String app_pass = "admin";
bool require_login = true;
String local_hostname = "yarrboard";

//keep track of our authenticated clients
const byte clientLimit = 8;
uint32_t authenticatedClientIDs[clientLimit];

AsyncWebSocket ws("/ws");
AsyncWebServer server(80);

//for tracking our message loop
int messageInterval = 250;
unsigned long previousMessageMillis = 0;
unsigned int handledMessages = 0;
unsigned int lastHandledMessages = 0;
unsigned long totalHandledMessages = 0;

void websocket_setup()
{
  //look up our board name
  if (preferences.isKey("boardName"))
    board_name = preferences.getString("boardName");

  //look up our username/password
  if (preferences.isKey("app_user"))
    app_user = preferences.getString("app_user");
  if (preferences.isKey("app_pass"))
    app_pass = preferences.getString("app_pass");
  if (preferences.isKey("require_login"))
    require_login = preferences.getBool("require_login");
  if (preferences.isKey("local_hostname"))
    local_hostname = preferences.getString("local_hostname");

  //config for our websocket server
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //we are only serving static files - big cache
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=2592000");
  server.begin();
}

void websocket_loop()
{
  //sometimes websocket clients die badly.
  ws.cleanupClients();

  //lookup our info periodically
  int messageDelta = millis() - previousMessageMillis;
  if (messageDelta >= messageInterval)
  {
    //read and send out our json update
    sendUpdate();
  
    //how fast are we?
    //Serial.print(messageDelta);
    //Serial.print("ms | msg/s: ");
    //Serial.print(handledMessages - lastHandledMessages);
    //Serial.println();

    //for keeping track.
    lastHandledMessages = handledMessages;
    previousMessageMillis = millis();
  }
}

void sendToAll(String jsonString)
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
    handleReceivedMessage((char *)data, client);
  }
}

void handleReceivedMessage(char *payload, AsyncWebSocketClient *client)
{
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, payload);

  //was there a problem, officer?
  if (err) {
    String error = "deserializeJson() failed with code ";
    error += err.c_str();
    sendErrorJSON(error, client);
  }

  //what is your command?
  String cmd = doc["cmd"];

  //keep track!
  handledMessages++;
  totalHandledMessages++;

  //change state?
  if (cmd.equals("set_state"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //is it a valid channel?
    byte cid = doc["id"];
    if (!assertValidChannel(cid, client))
      return;

    //is it enabled?
    if (!channelIsEnabled[cid])
    {
      sendErrorJSON("Channel is not enabled.", client);
      return;
    }

    //what is our new state?
    bool state = doc["value"];

    //keep track of how many toggles
    if (state && channelState[cid] != state)
      channelStateChangeCount[cid]++;

    //record our new state
    channelState[cid] = state;

    //reset soft fuse when we turn on
    if (state)
      channelTripped[cid] = false;

    //change our output pin to reflect
    updateChannelState(cid);
  }
  //change duty cycle?
  else if (cmd.equals("set_duty"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //is it a valid channel?
    byte cid = doc["id"];
    if (!assertValidChannel(cid, client))
      return;

    //is it enabled?
    if (!channelIsEnabled[cid])
    {
      sendErrorJSON("Channel is not enabled.", client);
      return;
    }

    float value = doc["value"];

    //what do we hate?  va-li-date!
    if (value < 0)
      sendErrorJSON("Duty cycle must be >= 0", client);
    else if (value > 1)
      sendErrorJSON("Duty cycle must be <= 1", client);
    else {
      channelDutyCycle[cid] = value;

      //save to our storage
      String prefIndex = "cDuty" + String(cid);
      if (millis() - channelLastDutyCycleUpdate[cid] > 1000)
      {
        preferences.putFloat(prefIndex.c_str(), value);
        channelDutyCycleIsThrottled[cid] = false;
      }
      //make a note so we can save later.
      else
        channelDutyCycleIsThrottled[cid] = true;

      //we want the clock to reset every time we change the duty cycle
      //this way, long led fading sessions are only one write.
      channelLastDutyCycleUpdate[cid] = millis();
    }

    //change our output pin to reflect
    updateChannelState(cid);
  }
  //change a board name?
  else if (cmd.equals("set_boardname"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    String value = doc["value"];
    if (value.length() > 30)
      sendErrorJSON("Maximum board name length is 30 characters.", client);
    else
    {
      //update variable
      board_name = value;

      //save to our storage
      preferences.putString("boardName", value);

      //give them the updated config
      sendConfigJSON(client);
    }
  }
  //change a channel name?
  else if (cmd.equals("set_channelname"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //is it a valid channel?
    byte cid = doc["id"];
    if (!assertValidChannel(cid, client))
      return;

    //validate yo mamma.
    String value = doc["value"];
    if (value.length() > 30)
      sendErrorJSON("Maximum channel name length is 30 characters.", client);
    else 
    {
      channelNames[cid] = value;

      //save to our storage
      String prefIndex = "cName" + String(cid);
      preferences.putString(prefIndex.c_str(), value);

      //give them the updated config
      sendConfigJSON(client);
    }
  }
  //change a channels dimmability?
  else if (cmd.equals("set_dimmable"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //is it a valid channel?
    byte cid = doc["id"];
    if (!assertValidChannel(cid, client))
      return;

    //save right nwo.
    bool value = doc["value"];
    channelIsDimmable[cid] = value;

    //save to our storage
    String prefIndex = "cDimmable" + String(cid);
    preferences.putBool(prefIndex.c_str(), value);

    //give them the updated config
    sendConfigJSON(client);
  }
  //change a channels dimmability?
  else if (cmd.equals("set_enabled"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //is it a valid channel?
    byte cid = doc["id"];
    if (!assertValidChannel(cid, client))
      return;

    //save right nwo.
    bool value = doc["value"];
    channelIsEnabled[cid] = value;

    //save to our storage
    String prefIndex = "cEnabled" + String(cid);
    preferences.putBool(prefIndex.c_str(), value);

    //give them the updated config
    sendConfigJSON(client);
  }  
  //change a channels soft fuse?
  else if (cmd.equals("set_soft_fuse"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //is it a valid channel?
    byte cid = doc["id"];
    if (!assertValidChannel(cid, client))
      return;

    //i crave validation!
    float value = doc["value"];
    value = constrain(value, 0.01, 20.0);

    //save right nwo.
    channelSoftFuseAmperage[cid] = value;

    //save to our storage
    String prefIndex = "cSoftFuse" + String(cid);
    preferences.putFloat(prefIndex.c_str(), value);

    //give them the updated config
    sendConfigJSON(client);
  }
  //get our config?
  else if (cmd.equals("get_config"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    sendConfigJSON(client);
  }
  //networking?
  else if (cmd.equals("get_network_config"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    sendNetworkConfigJSON(client);
  }
  //setup networking?
  else if (cmd.equals("set_network_config"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //clear our first boot flag since they submitted the network page.
    is_first_boot = false;

    //get our data
    String new_wifi_mode = doc["wifi_mode"].as<String>();
    String new_wifi_ssid = doc["wifi_ssid"].as<String>();
    String new_wifi_pass = doc["wifi_pass"].as<String>();
    local_hostname = doc["local_hostname"].as<String>();
    app_user = doc["app_user"].as<String>();
    app_pass = doc["app_pass"].as<String>();
    require_login = doc["require_login"];

    //make sure we can connect before we save
    if (new_wifi_mode.equals("client"))
    {
      //did we change username/password?
      if (!new_wifi_ssid.equals(wifi_ssid) || !new_wifi_pass.equals(wifi_pass))
      {
        //try connecting.
        if (connectToWifi(new_wifi_ssid, new_wifi_pass))
        {
          //let the client know.
          sendSuccessJSON("Connected to new WiFi.", client);

          //changing modes?
          if (wifi_mode.equals("ap"))
            WiFi.softAPdisconnect();

          //save to flash
          preferences.putString("wifi_mode", new_wifi_mode);
          preferences.putString("wifi_ssid", new_wifi_ssid);
          preferences.putString("wifi_pass", new_wifi_pass);

          //save for local use
          wifi_mode = new_wifi_mode;
          wifi_ssid = new_wifi_ssid;
          wifi_pass = new_wifi_pass;
        }
        //nope, setup our wifi back to default.
        else
          sendErrorJSON("Can't connect to new WiFi.", client);
      }
      else
        sendSuccessJSON("Network settings updated.", client);
    }
    //okay, AP mode is easier
    else
    {
      sendSuccessJSON("AP mode successful, please connect to new network.", client);

      //changing modes?
      //if (wifi_mode.equals("client"))
      //  WiFi.disconnect();

      //save to flash
      preferences.putString("wifi_mode", new_wifi_mode);
      preferences.putString("wifi_ssid", new_wifi_ssid);
      preferences.putString("wifi_pass", new_wifi_pass);

      //save for local use.
      wifi_mode = new_wifi_mode;
      wifi_ssid = new_wifi_ssid;
      wifi_pass = new_wifi_pass;

      //switch us into AP mode
      setupWifi();
    }

    //no special cases here.
    preferences.putString("local_hostname", local_hostname);
    preferences.putString("app_user", app_user);
    preferences.putString("app_pass", app_pass);
    preferences.putBool("require_login", require_login);
  }  
  //get our stats?
  else if (cmd.equals("get_stats"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    sendStatsJSON(client);
  }
  //get our config?
  else if (cmd.equals("login"))
  {
    if (!require_login)
    {
      sendErrorJSON("Login not required.", client);
      return;
    }

    String myuser = doc["user"];
    String mypass = doc["pass"];

    //morpheus... i'm in.
    if (myuser.equals(app_user) && mypass.equals(app_pass))
    {
      //declare outside, so we can count.
      byte i;
      for (i=0; i<clientLimit; i++)
      {
        //did we find an empty slot?
        if (authenticatedClientIDs[i] == 0)
        {
          authenticatedClientIDs[i] = client->id();
          break;
        }

        //are we already authenticated?
        if (authenticatedClientIDs[i] == client->id())
          break;
      }

      //did we not find a spot?
      if (i == clientLimit)
      {
        sendErrorJSON("Too many connections.", client);
        client->close();
      }
      //we must be good then.
      else
      {
        sendSuccessJSON("Login successful.", client);
      }
    }
    //gtfo.
    else
      sendErrorJSON("Wrong username/password.", client);
  }
  //restart
  else if (cmd.equals("restart"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;
    
    //do it.
    ESP.restart();
  }
  else if (cmd.equals("factory_reset"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //delete all our prefs
    preferences.clear();
    preferences.end();

    //restart the board.
    ESP.restart();
  }
  //ping for heartbeat
  else if (cmd.equals("ping"))
  {
    String jsonString;  // Temporary storage for the JSON String
    StaticJsonDocument<16> doc;

    // create an object
    JsonObject object = doc.to<JsonObject>();
    object["pong"] = millis();

    // serialize the object and save teh result to teh string variable.
    serializeJson(doc, jsonString);
    if (client->canSend())
      ws.text(client->id(), jsonString);
  }
  else if (cmd.equals("ota_start"))
  {
    //clean runs only.
    if (!assertLoggedIn(client))
      return;

    //look for new firmware
    bool updatedNeeded = FOTA.execHTTPcheck();
    if (updatedNeeded)
      doOTAUpdate = true;
    else
      sendErrorJSON("Firmware already up to date.", client);
  }
  //unknown command.
  else
  {
    Serial.print("Invalid command: ");
    Serial.println(cmd);
    sendErrorJSON("Invalid command.", client);
  }
}

bool assertLoggedIn(AsyncWebSocketClient *client)
{
  //only if required by law!
  if (!require_login)
    return true;

  //are they in our auth array?
  for (byte i=0; i<clientLimit; i++)
    if (authenticatedClientIDs[i] == client->id())
      return true;

  //let them know.
  sendErrorJSON("You must be logged in.", client);

  return false;
}

//is it a valid channel?
bool assertValidChannel(byte cid, AsyncWebSocketClient *client)
{
  if (cid < 0 || cid >= CHANNEL_COUNT) {
    sendErrorJSON("Invalid channel id", client);
    return false;
  }

  return true;
}

void sendSuccessJSON(String success, AsyncWebSocketClient *client) {
  String jsonString;  // Temporary storage for the JSON String
  StaticJsonDocument<256> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();
  object["success"] = success;

  // serialize the object and send it
  serializeJson(doc, jsonString);

  if (client->canSend())
    ws.text(client->id(), jsonString);
}

void sendErrorJSON(String error, AsyncWebSocketClient *client)
{
  String jsonString;  // Temporary storage for the JSON String
  StaticJsonDocument<256> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();
  object["error"] = error;

  // serialize the object and send it
  serializeJson(doc, jsonString);

  if (client->canSend())
    ws.text(client->id(), jsonString);
}

void sendConfigJSON(AsyncWebSocketClient *client)
{
  String jsonString;
  StaticJsonDocument<2048> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  //our identifying info
  object["firmware_version"] = FIRMWARE_VERSION;
  object["hardware_version"] = HARDWARE_VERSION;
  object["name"] = board_name;
  object["hostname"] = local_hostname;
  object["uuid"] = uuid;
  object["msg"] = "config";

  //do we want to flag it for config?
  if (is_first_boot)
    object["first_boot"] = true;

  //send our configuration
  for (byte i = 0; i < CHANNEL_COUNT; i++) {
    object["channels"][i]["id"] = i;
    object["channels"][i]["name"] = channelNames[i];
    object["channels"][i]["type"] = "mosfet";
    object["channels"][i]["enabled"] = channelIsEnabled[i];
    object["channels"][i]["hasPWM"] = true;
    object["channels"][i]["hasCurrent"] = true;
    object["channels"][i]["softFuse"] = round2(channelSoftFuseAmperage[i]);
    object["channels"][i]["isDimmable"] = channelIsDimmable[i];
  }

  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString);

  //Serial.println( jsonString );

  // send the JSON object through the websocket
  if (client->canSend())
    ws.text(client->id(), jsonString);
}

void sendNetworkConfigJSON(AsyncWebSocketClient *client)
{
  String jsonString;
  StaticJsonDocument<256> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  //our identifying info
  object["msg"] = "network_config";
  object["wifi_mode"] = wifi_mode;
  object["wifi_ssid"] = wifi_ssid;
  object["wifi_pass"] = wifi_pass;
  object["local_hostname"] = local_hostname;
  object["require_login"] = require_login;
  object["app_user"] = app_user;
  object["app_pass"] = app_pass;

  //serialize the object and send it.
  serializeJson(doc, jsonString);

  if (client->canSend())
    ws.text(client->id(), jsonString);
}

void sendStatsJSON(AsyncWebSocketClient *client)
{
  //stuff for working with json
  String jsonString;
  StaticJsonDocument<1536> doc;
  JsonObject object = doc.to<JsonObject>();

  //some basic statistics and info
  object["msg"] = "stats";
  object["uuid"] = uuid;
  object["messages"] = totalHandledMessages;
  object["uptime"] = millis();
  object["heap_size"] = ESP.getHeapSize();
  object["free_heap"] = ESP.getFreeHeap();
  object["min_free_heap"] = ESP.getMinFreeHeap();
  object["max_alloc_heap"] = ESP.getMaxAllocHeap();
  object["rssi"] = WiFi.RSSI();
  object["bus_voltage"] = busVoltage;

  //what is our IP address?
  if (wifi_mode.equals("ap"))
    object["ip_address"] = apIP;
  else
    object["ip_address"] = WiFi.localIP();

  //info about each of our channels
  for (byte i = 0; i < CHANNEL_COUNT; i++) {
    object["channels"][i]["id"] = i;
    object["channels"][i]["name"] = channelNames[i];
    object["channels"][i]["aH"] = channelAmpHour[i];
    object["channels"][i]["wH"] = channelWattHour[i];
    object["channels"][i]["state_change_count"] = channelStateChangeCount[i];
    object["channels"][i]["soft_fuse_trip_count"] = channelSoftFuseTripCount[i];
  }

  //okay prep our json and send it off
  serializeJson(doc, jsonString);

  if (client->canSend())
    ws.text(client->id(), jsonString);
}

void sendUpdate()
{
  StaticJsonDocument<3000> doc;
  String jsonString;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["msg"] = "update";
  object["bus_voltage"] = busVoltage;

  /*
    if (getLocalTime(&timeinfo)) {
      char buffer[80];
      strftime(buffer, 80, "%FT%T%z", &timeinfo);
      object["time"] = buffer;
    }
  */

  for (byte i = 0; i < CHANNEL_COUNT; i++) {
    object["channels"][i]["id"] = i;
    object["channels"][i]["state"] = channelState[i];
    if (channelIsDimmable[i])
      object["channels"][i]["duty"] = round2(channelDutyCycle[i]);

    object["channels"][i]["current"] = round2(channelAmperage[i]);
    object["channels"][i]["aH"] = round3(channelAmpHour[i]);
    object["channels"][i]["wH"] = round3(channelWattHour[i]);

    if (channelTripped[i])
      object["channels"][i]["soft_fuse_tripped"] = true;
  }

  //send it.
  serializeJson(doc, jsonString);
  sendToAll(jsonString);
}