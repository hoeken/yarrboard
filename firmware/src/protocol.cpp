/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "protocol.h"

String board_name = "Yarrboard";

//for tracking our message loop
unsigned long previousMessageMillis = 0;
unsigned int lastHandledMessages = 0;
unsigned int handledMessages = 0;
unsigned long totalHandledMessages = 0;

void protocol_setup()
{
  if (app_enable_serial)
  {
    char jsonBuffer[MAX_JSON_LENGTH];
    generateConfigJSON(jsonBuffer);
    Serial.println(jsonBuffer);
  }
}

void protocol_loop()
{
  //lookup our info periodically
  int messageDelta = millis() - previousMessageMillis;
  if (messageDelta >= YB_UPDATE_FREQUENCY)
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

  //any serial port customers?
  if (app_enable_serial)
  {
    if (Serial.available() > 0)
      handleSerialJson();
  }
}

void handleSerialJson()
{
  String input = Serial.readStringUntil('\n');

  StaticJsonDocument<1024> json;
  DeserializationError err = deserializeJson(json, input);
  JsonObject doc = json.as<JsonObject>();

  char jsonBuffer[MAX_JSON_LENGTH];

  //is there a problem, officer?
  if (err)
  {
    String error = "deserializeJson() failed with code ";
    error += err.c_str();
    generateErrorJSON(jsonBuffer, error);
    Serial.println(jsonBuffer);
  }
  else
  {
    handleReceivedJSON(doc, jsonBuffer, YBP_MODE_SERIAL, 0);
    Serial.println(jsonBuffer);
  }
}

void handleReceivedJSON(const JsonObject &doc, char *output, byte mode, uint32_t client_id)
{
  //what is your command?
  String cmd = doc["cmd"];

  //keep track!
  handledMessages++;
  totalHandledMessages++;

  //only pages with no login requirements
  if (cmd.equals("login") || cmd.equals("ping"))
  {
    if (cmd.equals("login"))
      return handleLogin(doc, output, mode, client_id);
    else if (cmd.equals("ping"))
      return generatePongJSON(output);
  }
  else
  {
    //need to be logged in here.
    if (!isLoggedIn(doc, mode, client_id))
        return generateLoginRequiredJSON(output);

    //what is your command?
    if (cmd.equals("set_boardname"))
      return handleSetBoardName(doc, output);   
    else if (cmd.equals("set_channel"))
      return handleSetChannel(doc, output);
    else if (cmd.equals("get_config"))
      return generateConfigJSON(output);
    else if (cmd.equals("get_network_config"))
      return generateNetworkConfigJSON(output);
    else if (cmd.equals("set_network_config"))
      return handleSetNetworkConfig(doc, output);
    else if (cmd.equals("get_app_config"))
      return generateAppConfigJSON(output);
    else if (cmd.equals("set_app_config"))
      return handleSetAppConfig(doc, output);
    else if (cmd.equals("get_stats"))
      return generateStatsJSON(output);
    else if (cmd.equals("get_update"))
      return generateUpdateJSON(output);
    else if (cmd.equals("restart"))
      return handleRestart(doc, output);
    else if (cmd.equals("factory_reset"))
      return handleFactoryReset(doc, output);
    else if (cmd.equals("ota_start"))
      return handleOTAStart(doc, output);
    else
      return generateErrorJSON(output, "Invalid command.");
  }

  //unknown command.
  return generateErrorJSON(output, "Malformed json.");
}

void handleSetBoardName(const JsonObject& doc, char * output)
{
    String value = doc["value"];
    if (value.length() > 30)
      return generateErrorJSON(output, "Maximum board name length is 30 characters.");

    //update variable
    board_name = value;

    //save to our storage
    preferences.putString("boardName", value);

    //give them the updated config
    return generateConfigJSON(output);
}

void handleSetChannel(const JsonObject& doc, char * output)
{
  //id is required
  if (!doc.containsKey("id"))
    return generateErrorJSON(output, "'id' is a required parameter");

  //is it a valid channel?
  byte cid = doc["id"];
  if (!isValidChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  //change state
  if (doc.containsKey("state"))
  {
    //is it enabled?
    if (!channelIsEnabled[cid])
      return generateErrorJSON(output, "Channel is not enabled.");

    //what is our new state?
    bool state = doc["state"];

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

  //our duty cycle
  if (doc.containsKey("duty"))
  {
    //is it enabled?
    if (!channelIsEnabled[cid])
      return generateErrorJSON(output, "Channel is not enabled.");

    float duty = doc["duty"];

    //what do we hate?  va-li-date!
    if (duty < 0)
      return generateErrorJSON(output, "Duty cycle must be >= 0");
    else if (duty > 1)
      return generateErrorJSON(output, "Duty cycle must be <= 1");
    else {
      channelDutyCycle[cid] = duty;

      //save to our storage
      String prefIndex = "cDuty" + String(cid);
      if (millis() - channelLastDutyCycleUpdate[cid] > 1000)
      {
        preferences.putFloat(prefIndex.c_str(), duty);
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

  //channel name
  if (doc.containsKey("name"))
  {
    //validate yo mamma.
    String name = doc["name"];
    if (name.length() > 30)
      return generateErrorJSON(output, "Maximum channel name length is 30 characters.");

    channelNames[cid] = name;

    //save to our storage
    String prefIndex = "cName" + String(cid);
    preferences.putString(prefIndex.c_str(), name);

    //give them the updated config
    return generateConfigJSON(output);
  }

  //dimmability
  if (doc.containsKey("isDimmable"))
  {
    bool isDimmable = doc["isDimmable"];
    channelIsDimmable[cid] = isDimmable;

    //save to our storage
    String prefIndex = "cDimmable" + String(cid);
    preferences.putBool(prefIndex.c_str(), isDimmable);

    //give them the updated config
    return generateConfigJSON(output);
  }

  //enabled
  if (doc.containsKey("enabled"))
  {
    //save right nwo.
    bool enabled = doc["enabled"];
    channelIsEnabled[cid] = enabled;

    //save to our storage
    String prefIndex = "cEnabled" + String(cid);
    preferences.putBool(prefIndex.c_str(), enabled);

    //give them the updated config
    return generateConfigJSON(output);
  }

  //soft fuse
  if (doc.containsKey("softFuse"))
  {
    //i crave validation!
    float softFuse = doc["softFuse"];
    softFuse = constrain(softFuse, 0.01, 20.0);

    //save right nwo.
    channelSoftFuseAmperage[cid] = softFuse;

    //save to our storage
    String prefIndex = "cSoftFuse" + String(cid);
    preferences.putFloat(prefIndex.c_str(), softFuse);

    //give them the updated config
    return generateConfigJSON(output);
  }

  return generateOKJSON(output);
}

void handleSetNetworkConfig(const JsonObject& doc, char * output)
{
    //clear our first boot flag since they submitted the network page.
    is_first_boot = false;

    //get our data
    String new_wifi_mode = doc["wifi_mode"].as<String>();
    String new_wifi_ssid = doc["wifi_ssid"].as<String>();
    String new_wifi_pass = doc["wifi_pass"].as<String>();
    local_hostname = doc["local_hostname"].as<String>();

    //no special cases here.
    preferences.putString("local_hostname", local_hostname);

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
          return generateSuccessJSON(output, "Connected to new WiFi.");

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
          return generateErrorJSON(output, "Can't connect to new WiFi.");
      }
      else
        return generateSuccessJSON(output, "Network settings updated.");
    }
    //okay, AP mode is easier
    else
    {
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

      return generateSuccessJSON(output, "AP mode successful, please connect to new network.");
    }

}

void handleSetAppConfig(const JsonObject& doc, char * output)
{
    //get our data
    app_user = doc["app_user"].as<String>();
    app_pass = doc["app_pass"].as<String>();
    require_login = doc["require_login"];
    app_enable_api = doc["app_enable_api"];
    app_enable_serial = doc["app_enable_serial"];

    //no special cases here.
    preferences.putString("app_user", app_user);
    preferences.putString("app_pass", app_pass);
    preferences.putBool("require_login", require_login);  
    preferences.putBool("appEnableApi", app_enable_api);  
    preferences.putBool("appEnableSerial", app_enable_serial);  
}

void handleLogin(const JsonObject& doc, char * output, byte mode, uint32_t client_id)
{
    if (!require_login)
      return generateErrorJSON(output, "Login not required.");

    String myuser = doc["user"];
    String mypass = doc["pass"];

    //morpheus... i'm in.
    if (myuser.equals(app_user) && mypass.equals(app_pass))
    {
        //check to see if there's room for us.
        if (mode == YBP_MODE_WEBSOCKET)
            if (!logClientIn(client_id))
                return generateErrorJSON(output, "Too many connections.");

        return generateSuccessJSON(output, "Login successful.");
    }

    //gtfo.
    return generateErrorJSON(output, "Wrong username/password.");
}

void handleRestart(const JsonObject& doc, char * output)
{
  ESP.restart();
}

void handleFactoryReset(const JsonObject& doc, char * output)
{
  //delete all our prefs
  preferences.clear();
  preferences.end();

  //restart the board.
  ESP.restart();
}

void handleOTAStart(const JsonObject& doc, char * output)
{
  //look for new firmware
  bool updatedNeeded = FOTA.execHTTPcheck();
  if (updatedNeeded)
    doOTAUpdate = true;
  else
    return generateErrorJSON(output, "Firmware already up to date.");
}

void generateStatsJSON(char * jsonBuffer)
{
  //stuff for working with json
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

  //info about each of our fans
  for (byte i = 0; i < FAN_COUNT; i++) {
    object["fans"][i]["rpm"] = fans_last_rpm[i];
    object["fans"][i]["pwm"] = fans_last_pwm[i];
  }

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
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateUpdateJSON(char * jsonBuffer)
{
  // create an object
  StaticJsonDocument<3000> doc;
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
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateConfigJSON(char * jsonBuffer)
{
  // create an object
  StaticJsonDocument<2048> doc;
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

  //send it.
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateNetworkConfigJSON(char * jsonBuffer)
{
  // create an object
  StaticJsonDocument<256> doc;
  JsonObject object = doc.to<JsonObject>();

  //our identifying info
  object["msg"] = "network_config";
  object["wifi_mode"] = wifi_mode;
  object["wifi_ssid"] = wifi_ssid;
  object["wifi_pass"] = wifi_pass;
  object["local_hostname"] = local_hostname;

  //send it.
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateAppConfigJSON(char * jsonBuffer)
{
  // create an object
  StaticJsonDocument<256> doc;
  JsonObject object = doc.to<JsonObject>();

  //our identifying info
  object["msg"] = "app_config";
  object["require_login"] = require_login;
  object["app_user"] = app_user;
  object["app_pass"] = app_pass;
  object["app_enable_api"] = app_enable_api;
  object["app_enable_serial"] = app_enable_serial;

  //send it.
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateOTAProgressUpdateJSON(char * jsonBuffer, float progress, int partition)
{
  // create an object
  StaticJsonDocument<96> doc;
  JsonObject object = doc.to<JsonObject>();

  object["msg"] = "ota_progress";

  if (partition == U_SPIFFS)
    object["partition"] = "spiffs";
  else
    object["partition"] = "firmware";

  object["progress"] = round2(progress);

  //send it.
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateOTAProgressFinishedJSON(char * jsonBuffer)
{
  // create an object
  StaticJsonDocument<96> doc;
  JsonObject object = doc.to<JsonObject>();

  object["msg"] = "ota_finished";

  //send it.
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateErrorJSON(char * jsonBuffer, String error)
{
  // create an object
  StaticJsonDocument<256> doc;
  JsonObject object = doc.to<JsonObject>();

  object["error"] = error;

  //send it.
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateSuccessJSON(char * jsonBuffer, String success)
{
  // create an object
  StaticJsonDocument<256> doc;
  JsonObject object = doc.to<JsonObject>();

  object["success"] = success;

  //send it.
  serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

bool isLoggedIn(const JsonObject& doc, byte mode, uint32_t client_id = 0)
{
  //also only if enabled
  if (!require_login)
    return true;

  //login only required for websockets.
  if (mode == YBP_MODE_WEBSOCKET)
    return isWebsocketClientLoggedIn(doc, client_id);
  else if (mode == YBP_MODE_HTTP)
    return isApiClientLoggedIn(doc);
  else if (mode == YBP_MODE_SERIAL)
    return isSerialClientLoggedIn(doc);
  else
    return false;
}

void generateLoginRequiredJSON(char * jsonBuffer)
{
  return generateErrorJSON(jsonBuffer, "You must be logged in.");
}

bool isValidChannel(byte cid)
{
  if (cid < 0 || cid >= CHANNEL_COUNT)
    return false;
  else
    return true;
}

void generatePongJSON(char * jsonBuffer)
{
    StaticJsonDocument<16> doc;

    // create an object
    JsonObject object = doc.to<JsonObject>();
    object["pong"] = millis();

    //send it.
    serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void generateOKJSON(char * jsonBuffer)
{
    StaticJsonDocument<16> doc;

    // create an object
    JsonObject object = doc.to<JsonObject>();
    object["ok"] = millis();

    //send it.
    serializeJson(doc, jsonBuffer, MAX_JSON_LENGTH);
}

void sendUpdate()
{
  char jsonBuffer[MAX_JSON_LENGTH];
  generateUpdateJSON(jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendOTAProgressUpdate(float progress, int partition)
{
  char jsonBuffer[MAX_JSON_LENGTH];
  generateOTAProgressUpdateJSON(jsonBuffer, progress, partition);
  sendToAll(jsonBuffer);
}

void sendOTAProgressFinished()
{
  char jsonBuffer[MAX_JSON_LENGTH];
  generateOTAProgressFinishedJSON(jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendToAll(char * jsonString)
{
  sendToAllWebsockets(jsonString);

  if (app_enable_serial)
    Serial.println(jsonString);
}