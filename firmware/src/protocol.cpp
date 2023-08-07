/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "protocol.h"

char board_name[YB_BOARD_NAME_LENGTH] = "Yarrboard";

//for tracking our message loop
unsigned long previousMessageMillis = 0;
unsigned int lastHandledMessages = 0;
unsigned int handledMessages = 0;
unsigned long totalHandledMessages = 0;

void protocol_setup()
{
  if (app_enable_serial)
  {
    StaticJsonDocument<3000> output;

    generateConfigJSON(output);
    serializeJson(output, Serial);
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
  StaticJsonDocument<1024> input;
  DeserializationError err = deserializeJson(input, Serial);

  StaticJsonDocument<3000> output;

  //ignore newlines with serial.
  if (err)
  {
    if (strcmp(err.c_str(), "EmptyInput"))
    {
      char error[64];
      sprintf(error, "deserializeJson() failed with code %s", err.c_str());
      generateErrorJSON(output, error);
      serializeJson(output, Serial);
    }
  }
  else
  {
    handleReceivedJSON(input, output, YBP_MODE_SERIAL, 0);

    //we can have empty responses
    if (output.size())
      serializeJson(output, Serial);    
  }
}

void handleReceivedJSON(JsonVariantConst input, JsonVariant output, byte mode, uint32_t client_id)
{
  //make sure its correct
  if (!input.containsKey("cmd"))
    return generateErrorJSON(output, "'cmd' is a required parameter.");

  //what is your command?
  const char* cmd = input["cmd"];

  //let the client keep track of messages
  if (input.containsKey("msgid"))
  {
    int msgid = input["msgid"];
    output["msgid"] = msgid;
  }

  //keep track!
  handledMessages++;
  totalHandledMessages++;

  //only pages with no login requirements
  if (!strcmp(cmd, "login") || !strcmp(cmd, "ping"))
  {
    if (!strcmp(cmd, "login"))
      return handleLogin(input, output, mode, client_id);
    else if (!strcmp(cmd, "ping"))
      return generatePongJSON(output);
  }
  else
  {
    //need to be logged in here.
    if (!isLoggedIn(input, mode, client_id))
        return generateLoginRequiredJSON(output);

    //what is your command?
    if (!strcmp(cmd, "set_boardname"))
      return handleSetBoardName(input, output);   
    else if (!strcmp(cmd, "set_channel"))
      return handleSetChannel(input, output);
    else if (!strcmp(cmd, "toggle_channel"))
      return handleToggleChannel(input, output);
    else if (!strcmp(cmd, "fade_channel"))
      return handleFadeChannel(input, output);
    else if (!strcmp(cmd, "get_config"))
      return generateConfigJSON(output);
    else if (!strcmp(cmd, "get_network_config"))
      return generateNetworkConfigJSON(output);
    else if (!strcmp(cmd, "set_network_config"))
      return handleSetNetworkConfig(input, output);
    else if (!strcmp(cmd, "get_app_config"))
      return generateAppConfigJSON(output);
    else if (!strcmp(cmd, "set_app_config"))
      return handleSetAppConfig(input, output);
    else if (!strcmp(cmd, "get_stats"))
      return generateStatsJSON(output);
    else if (!strcmp(cmd, "get_update"))
      return generateUpdateJSON(output);
    else if (!strcmp(cmd, "restart"))
      return handleRestart(input, output);
    else if (!strcmp(cmd, "factory_reset"))
      return handleFactoryReset(input, output);
    else if (!strcmp(cmd, "ota_start"))
      return handleOTAStart(input, output);
    else
      return generateErrorJSON(output, "Invalid command.");
  }

  //unknown command.
  return generateErrorJSON(output, "Malformed json.");
}

void handleSetBoardName(JsonVariantConst input, JsonVariant output)
{
    if (!input.containsKey("value"))
      return generateErrorJSON(output, "'value' is a required parameter");

    //is it too long?
    if (strlen(input["value"]) > YB_BOARD_NAME_LENGTH-1)
    {
      char error[50];
      sprintf(error, "Maximum board name length is %s characters.", YB_BOARD_NAME_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    //update variable
    strlcpy(board_name, input["value"] | "Yarrboard", sizeof(board_name));

    //save to our storage
    preferences.putString("boardName", board_name);

    //give them the updated config
    return generateConfigJSON(output);
}

void handleSetChannel(JsonVariantConst input, JsonVariant output)
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  //id is required
  if (!input.containsKey("id"))
    return generateErrorJSON(output, "'id' is a required parameter");

  //is it a valid channel?
  byte cid = input["id"];
  if (!isValidChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  //change state
  if (input.containsKey("state"))
  {
    //is it enabled?
    if (!channels[cid].isEnabled)
      return generateErrorJSON(output, "Channel is not enabled.");

    //what is our new state?
    bool state = input["state"];

    //keep track of how many toggles
    if (state && channels[cid].state != state)
      channels[cid].stateChangeCount++;

    //record our new state
    channels[cid].state = state;

    //reset soft fuse when we turn on
    if (state)
      channels[cid].tripped = false;

    //change our output pin to reflect
    channels[cid].updateOutput();
  }

  //our duty cycle
  if (input.containsKey("duty"))
  {
    //is it enabled?
    if (!channels[cid].isEnabled)
      return generateErrorJSON(output, "Channel is not enabled.");

    float duty = input["duty"];

    //what do we hate?  va-li-date!
    if (duty < 0)
      return generateErrorJSON(output, "Duty cycle must be >= 0");
    else if (duty > 1)
      return generateErrorJSON(output, "Duty cycle must be <= 1");

    //okay, we're good.
    channels[cid].setDuty(duty);

    //change our output pin to reflect
    channels[cid].updateOutput();
  }

  //channel name
  if (input.containsKey("name"))
  {
    //is it too long?
    if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH-1)
    {
      char error[50];
      sprintf(error, "Maximum channel name length is %s characters.", YB_CHANNEL_NAME_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    //save to our storage
    strlcpy(channels[cid].name, input["name"] | "Channel ?", sizeof(channels[cid].name));
    sprintf(prefIndex, "cName%d", cid);
    preferences.putString(prefIndex, channels[cid].name);

    //give them the updated config
    return generateConfigJSON(output);
  }

  //dimmability
  if (input.containsKey("isDimmable"))
  {
    bool isDimmable = input["isDimmable"];
    channels[cid].isDimmable = isDimmable;

    //save to our storage
    sprintf(prefIndex, "cDimmable%d", cid);
    preferences.putBool(prefIndex, isDimmable);

    //give them the updated config
    return generateConfigJSON(output);
  }

  //enabled
  if (input.containsKey("enabled"))
  {
    //save right nwo.
    bool enabled = input["enabled"];
    channels[cid].isEnabled = enabled;

    //save to our storage
    sprintf(prefIndex, "cEnabled%d", cid);
    preferences.putBool(prefIndex, enabled);

    //give them the updated config
    return generateConfigJSON(output);
  }

  //soft fuse
  if (input.containsKey("softFuse"))
  {
    //i crave validation!
    float softFuse = input["softFuse"];
    softFuse = constrain(softFuse, 0.01, 20.0);

    //save right nwo.
    channels[cid].softFuseAmperage = softFuse;

    //save to our storage
    sprintf(prefIndex, "cSoftFuse%d", cid);
    preferences.putFloat(prefIndex, softFuse);

    //give them the updated config
    return generateConfigJSON(output);
  }
}

void handleToggleChannel(JsonVariantConst input, JsonVariant output)
{
  //id is required
  if (!input.containsKey("id"))
    return generateErrorJSON(output, "'id' is a required parameter");

  //is it a valid channel?
  byte cid = input["id"];
  if (!isValidChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  //keep track of how many toggles
  channels[cid].stateChangeCount++;

  //record our new state
  channels[cid].state = !channels[cid].state;

  //reset soft fuse when we turn on
  if (channels[cid].state)
    channels[cid].tripped = false;

  //change our output pin to reflect
  channels[cid].updateOutput();
}

void handleFadeChannel(JsonVariantConst input, JsonVariant output)
{
  unsigned long start = micros();
  unsigned long t1, t2, t3, t4 = 0;

  //id is required
  if (!input.containsKey("id"))
    return generateErrorJSON(output, "'id' is a required parameter");
  if (!input.containsKey("duty"))
    return generateErrorJSON(output, "'duty' is a required parameter");
  if (!input.containsKey("millis"))
    return generateErrorJSON(output, "'millis' is a required parameter");

  //is it a valid channel?
  byte cid = input["id"];
  if (!isValidChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  float duty = input["duty"];

  //what do we hate?  va-li-date!
  if (duty < 0)
    return generateErrorJSON(output, "Duty cycle must be >= 0");
  else if (duty > 1)
    return generateErrorJSON(output, "Duty cycle must be <= 1");

  t1 = micros();

  //okay, we're good.
  channels[cid].setDuty(duty);

  t2 = micros();

  int fadeDelay = input["millis"] | 0;
  
  channels[cid].setFade(duty, fadeDelay);

  t3 = micros();

  unsigned long finish = micros();

  if (finish-start > 10000)
  {
    Serial.println("led fade");
    Serial.printf("params: %dus\n", t1-start); 
    Serial.printf("channelSetDuty: %dus\n", t2-t1); 
    Serial.printf("channelFade: %dus\n", t3-t2); 
    Serial.printf("total: %dus\n", finish-start);
    Serial.println();
  }
}

void handleSetNetworkConfig(JsonVariantConst input, JsonVariant output)
{
    //clear our first boot flag since they submitted the network page.
    is_first_boot = false;

    char error[50];

    //error checking
    if (!input.containsKey("wifi_mode"))
      return generateErrorJSON(output, "'wifi_mode' is a required parameter");
    if (!input.containsKey("wifi_ssid"))
      return generateErrorJSON(output, "'wifi_ssid' is a required parameter");
    if (!input.containsKey("wifi_pass"))
      return generateErrorJSON(output, "'wifi_pass' is a required parameter");
    if (!input.containsKey("local_hostname"))
      return generateErrorJSON(output, "'local_hostname' is a required parameter");

    //is it too long?
    if (strlen(input["wifi_ssid"]) > YB_WIFI_SSID_LENGTH-1)
    {
      sprintf(error, "Maximum wifi ssid length is %s characters.", YB_WIFI_SSID_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    if (strlen(input["wifi_pass"]) > YB_WIFI_PASSWORD_LENGTH-1)
    {
      sprintf(error, "Maximum wifi password length is %s characters.", YB_WIFI_PASSWORD_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    if (strlen(input["local_hostname"]) > YB_HOSTNAME_LENGTH-1)
    {
      sprintf(error, "Maximum hostname length is %s characters.", YB_HOSTNAME_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    //get our data
    char new_wifi_mode[16];
    char new_wifi_ssid[YB_WIFI_SSID_LENGTH];
    char new_wifi_pass[YB_WIFI_PASSWORD_LENGTH];
    
    strlcpy(new_wifi_mode, input["wifi_mode"] | "ap", sizeof(new_wifi_mode));
    strlcpy(new_wifi_ssid, input["wifi_ssid"] | "SSID", sizeof(new_wifi_ssid));
    strlcpy(new_wifi_pass, input["wifi_pass"] | "PASS", sizeof(new_wifi_pass));
    strlcpy(local_hostname, input["local_hostname"] | "yarrboard", sizeof(local_hostname));

    //no special cases here.
    preferences.putString("local_hostname", local_hostname);

    //make sure we can connect before we save
    if (!strcmp(new_wifi_mode, "client"))
    {
      //did we change username/password?
      if (strcmp(new_wifi_ssid, wifi_ssid) || strcmp(new_wifi_pass, wifi_pass))
      {
        //try connecting.
        if (connectToWifi(new_wifi_ssid, new_wifi_pass))
        {
          //let the client know.
          return generateSuccessJSON(output, "Connected to new WiFi.");

          //changing modes?
          if (!strcmp(wifi_mode, "ap"))
            WiFi.softAPdisconnect();

          //save to flash
          preferences.putString("wifi_mode", new_wifi_mode);
          preferences.putString("wifi_ssid", new_wifi_ssid);
          preferences.putString("wifi_pass", new_wifi_pass);

          //save for local use
          strlcpy(wifi_mode, new_wifi_mode, sizeof(wifi_mode));
          strlcpy(wifi_ssid, new_wifi_ssid, sizeof(wifi_ssid));
          strlcpy(wifi_pass, new_wifi_pass, sizeof(wifi_pass));
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
      strlcpy(wifi_mode, new_wifi_mode, sizeof(wifi_mode));
      strlcpy(wifi_ssid, new_wifi_ssid, sizeof(wifi_ssid));
      strlcpy(wifi_pass, new_wifi_pass, sizeof(wifi_pass));

      //switch us into AP mode
      setupWifi();

      return generateSuccessJSON(output, "AP mode successful, please connect to new network.");
    }
}

void handleSetAppConfig(JsonVariantConst input, JsonVariant output)
{
    if (!input.containsKey("app_user"))
      return generateErrorJSON(output, "'app_user' is a required parameter");
    if (!input.containsKey("app_pass"))
      return generateErrorJSON(output, "'app_pass' is a required parameter");

    //username length checker
    if (strlen(input["app_user"]) > YB_USERNAME_LENGTH-1)
    {
      char error[50];
      sprintf(error, "Maximum username length is %s characters.", YB_USERNAME_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    //password length checker
    if (strlen(input["app_pass"]) > YB_PASSWORD_LENGTH-1)
    {
      char error[50];
      sprintf(error, "Maximum password length is %s characters.", YB_PASSWORD_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    //get our data
    strlcpy(app_user, input["app_user"] | "admin", sizeof(app_user));
    strlcpy(app_pass, input["app_pass"] | "admin", sizeof(app_pass));
    require_login = input["require_login"];
    app_enable_api = input["app_enable_api"];
    app_enable_serial = input["app_enable_serial"];

    //no special cases here.
    preferences.putString("app_user", app_user);
    preferences.putString("app_pass", app_pass);
    preferences.putBool("require_login", require_login);  
    preferences.putBool("appEnableApi", app_enable_api);  
    preferences.putBool("appEnableSerial", app_enable_serial);  
}

void handleLogin(JsonVariantConst input, JsonVariant output, byte mode, uint32_t client_id)
{
    if (!require_login)
      return generateErrorJSON(output, "Login not required.");

  if (!input.containsKey("user"))
    return generateErrorJSON(output, "'user' is a required parameter");

  if (!input.containsKey("pass"))
    return generateErrorJSON(output, "'pass' is a required parameter");

    //init
    char myuser[YB_USERNAME_LENGTH];
    char mypass[YB_PASSWORD_LENGTH];
    strlcpy(myuser, input["user"] | "", sizeof(myuser));
    strlcpy(mypass, input["pass"] | "", sizeof(mypass));

    //morpheus... i'm in.
    if (!strcmp(app_user, myuser) && !strcmp(app_pass, mypass))
    {
        //check to see if there's room for us.
        if (mode == YBP_MODE_WEBSOCKET)
        {
          if (!logClientIn(client_id))
            return generateErrorJSON(output, "Too many connections.");
        }
        else if (mode == YBP_MODE_SERIAL)
          is_serial_authenticated = true;

        return generateSuccessJSON(output, "Login successful.");
    }

    //gtfo.
    return generateErrorJSON(output, "Wrong username/password.");
}

void handleRestart(JsonVariantConst input, JsonVariant output)
{
  ESP.restart();
}

void handleFactoryReset(JsonVariantConst input, JsonVariant output)
{
  //delete all our prefs
  preferences.clear();
  preferences.end();

  //restart the board.
  ESP.restart();
}

void handleOTAStart(JsonVariantConst input, JsonVariant output)
{
  //look for new firmware
  bool updatedNeeded = FOTA.execHTTPcheck();
  if (updatedNeeded)
    doOTAUpdate = true;
  else
    return generateErrorJSON(output, "Firmware already up to date.");
}

void generateStatsJSON(JsonVariant output)
{
  //some basic statistics and info
  output["msg"] = "stats";
  output["uuid"] = uuid;
  output["messages"] = totalHandledMessages;
  output["uptime"] = millis();
  output["heap_size"] = ESP.getHeapSize();
  output["free_heap"] = ESP.getFreeHeap();
  output["min_free_heap"] = ESP.getMinFreeHeap();
  output["max_alloc_heap"] = ESP.getMaxAllocHeap();
  output["rssi"] = WiFi.RSSI();
  output["bus_voltage"] = busVoltage;

  //what is our IP address?
  if (!strcmp(wifi_mode, "ap"))
    output["ip_address"] = apIP;
  else
    output["ip_address"] = WiFi.localIP();

  //info about each of our fans
  for (byte i = 0; i < FAN_COUNT; i++) {
    output["fans"][i]["rpm"] = fans_last_rpm[i];
    output["fans"][i]["pwm"] = fans_last_pwm[i];
  }

  //info about each of our channels
  for (byte i = 0; i < CHANNEL_COUNT; i++) {
    output["channels"][i]["id"] = i;
    output["channels"][i]["name"] = channels[i].name;
    output["channels"][i]["aH"] = channels[i].ampHours;
    output["channels"][i]["wH"] = channels[i].wattHours;
    output["channels"][i]["state_change_count"] = channels[i].stateChangeCount;
    output["channels"][i]["soft_fuse_trip_count"] = channels[i].softFuseTripCount;
  }
}

void generateUpdateJSON(JsonVariant output)
{
  output["msg"] = "update";
  output["bus_voltage"] = busVoltage;

  /*
    if (getLocalTime(&timeinfo)) {
      char buffer[80];
      strftime(buffer, 80, "%FT%T%z", &timeinfo);
      output["time"] = buffer;
    }
  */

  for (byte i = 0; i < CHANNEL_COUNT; i++) {
    output["channels"][i]["id"] = i;
    output["channels"][i]["state"] = channels[i].state;
    if (channels[i].isDimmable)
      output["channels"][i]["duty"] = round2(channels[i].dutyCycle);

    output["channels"][i]["current"] = round2(channels[i].amperage);
    output["channels"][i]["aH"] = round3(channels[i].ampHours);
    output["channels"][i]["wH"] = round3(channels[i].wattHours);

    if (channels[i].tripped)
      output["channels"][i]["soft_fuse_tripped"] = true;
  }
}

void generateConfigJSON(JsonVariant output)
{
  //our identifying info
  output["firmware_version"] = FIRMWARE_VERSION;
  output["hardware_version"] = HARDWARE_VERSION;
  output["name"] = board_name;
  output["hostname"] = local_hostname;
  output["uuid"] = uuid;
  output["msg"] = "config";

  //do we want to flag it for config?
  if (is_first_boot)
    output["first_boot"] = true;

  //send our configuration
  for (byte i = 0; i < CHANNEL_COUNT; i++) {
    output["channels"][i]["id"] = i;
    output["channels"][i]["name"] = channels[i].name;
    output["channels"][i]["type"] = "mosfet";
    output["channels"][i]["enabled"] = channels[i].isEnabled;
    output["channels"][i]["hasPWM"] = true;
    output["channels"][i]["hasCurrent"] = true;
    output["channels"][i]["softFuse"] = round2(channels[i].softFuseAmperage);
    output["channels"][i]["isDimmable"] = channels[i].isDimmable;
  }
}

void generateNetworkConfigJSON(JsonVariant output)
{
  //our identifying info
  output["msg"] = "network_config";
  output["wifi_mode"] = wifi_mode;
  output["wifi_ssid"] = wifi_ssid;
  output["wifi_pass"] = wifi_pass;
  output["local_hostname"] = local_hostname;
}

void generateAppConfigJSON(JsonVariant output)
{
  //our identifying info
  output["msg"] = "app_config";
  output["require_login"] = require_login;
  output["app_user"] = app_user;
  output["app_pass"] = app_pass;
  output["app_enable_api"] = app_enable_api;
  output["app_enable_serial"] = app_enable_serial;
}

void generateOTAProgressUpdateJSON(JsonVariant output, float progress, int partition)
{
  output["msg"] = "ota_progress";

  if (partition == U_SPIFFS)
    output["partition"] = "spiffs";
  else
    output["partition"] = "firmware";

  output["progress"] = round2(progress);
}

void generateOTAProgressFinishedJSON(JsonVariant output)
{
  output["msg"] = "ota_finished";
}

void generateErrorJSON(JsonVariant output, const char* error)
{
  output["status"] = "error";
  output["message"] = error;
}

void generateSuccessJSON(JsonVariant output, const char* success)
{
  output["status"] = "success";
  output["message"] = success;
}

bool isLoggedIn(JsonVariantConst input, byte mode, uint32_t client_id = 0)
{
  //also only if enabled
  if (!require_login)
    return true;

  //login only required for websockets.
  if (mode == YBP_MODE_WEBSOCKET)
    return isWebsocketClientLoggedIn(input, client_id);
  else if (mode == YBP_MODE_HTTP)
    return isApiClientLoggedIn(input);
  else if (mode == YBP_MODE_SERIAL)
    return isSerialClientLoggedIn(input);
  else
    return false;
}

void generateLoginRequiredJSON(JsonVariant output)
{
  generateErrorJSON(output, "You must be logged in.");
}

bool isValidChannel(byte cid)
{
  if (cid < 0 || cid >= CHANNEL_COUNT)
    return false;
  else
    return true;
}

void generatePongJSON(JsonVariant output)
{
  output["pong"] = millis();
}

void sendUpdate()
{
  StaticJsonDocument<3000> output;
  char jsonBuffer[MAX_JSON_LENGTH];

  generateUpdateJSON(output);

  serializeJson(output, jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendOTAProgressUpdate(float progress, int partition)
{
  StaticJsonDocument<256> output;
  char jsonBuffer[256];

  generateOTAProgressUpdateJSON(output, progress, partition);

  serializeJson(output, jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendOTAProgressFinished()
{
  StaticJsonDocument<256> output;
  char jsonBuffer[256];

  generateOTAProgressFinishedJSON(output);

  serializeJson(output, jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendToAll(const char * jsonString)
{
  sendToAllWebsockets(jsonString);

  if (app_enable_serial)
    Serial.println(jsonString);
}