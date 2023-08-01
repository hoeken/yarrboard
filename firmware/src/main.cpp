/*
  Yarrboard v1.0.0
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3

*/

#include <Preferences.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "time.h"
#include "sntp.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp32FOTA.hpp>
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "utility.h"
#include "adc.h"

//identify yourself!
String uuid;
String board_name = "Yarrboard";
bool is_first_boot = true;

//for our OTA updates
esp32FOTA esp32FOTA("esp32-fota-http", VERSION);
const char* manifest_url = "https://raw.githubusercontent.com/hoeken/yarrboard/main/firmware/firmware.json";

const char* root_ca = R"ROOT_CA(
-----BEGIN CERTIFICATE-----
MIIEvjCCA6agAwIBAgIQBtjZBNVYQ0b2ii+nVCJ+xDANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0yMTA0MTQwMDAwMDBaFw0zMTA0MTMyMzU5NTlaME8xCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxKTAnBgNVBAMTIERpZ2lDZXJ0IFRMUyBS
U0EgU0hBMjU2IDIwMjAgQ0ExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC
AQEAwUuzZUdwvN1PWNvsnO3DZuUfMRNUrUpmRh8sCuxkB+Uu3Ny5CiDt3+PE0J6a
qXodgojlEVbbHp9YwlHnLDQNLtKS4VbL8Xlfs7uHyiUDe5pSQWYQYE9XE0nw6Ddn
g9/n00tnTCJRpt8OmRDtV1F0JuJ9x8piLhMbfyOIJVNvwTRYAIuE//i+p1hJInuW
raKImxW8oHzf6VGo1bDtN+I2tIJLYrVJmuzHZ9bjPvXj1hJeRPG/cUJ9WIQDgLGB
Afr5yjK7tI4nhyfFK3TUqNaX3sNk+crOU6JWvHgXjkkDKa77SU+kFbnO8lwZV21r
eacroicgE7XQPUDTITAHk+qZ9QIDAQABo4IBgjCCAX4wEgYDVR0TAQH/BAgwBgEB
/wIBADAdBgNVHQ4EFgQUt2ui6qiqhIx56rTaD5iyxZV2ufQwHwYDVR0jBBgwFoAU
A95QNVbRTLtm8KPiGxvDl7I90VUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQG
CCsGAQUFBwMBBggrBgEFBQcDAjB2BggrBgEFBQcBAQRqMGgwJAYIKwYBBQUHMAGG
GGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBABggrBgEFBQcwAoY0aHR0cDovL2Nh
Y2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9vdENBLmNydDBCBgNV
HR8EOzA5MDegNaAzhjFodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vRGlnaUNlcnRH
bG9iYWxSb290Q0EuY3JsMD0GA1UdIAQ2MDQwCwYJYIZIAYb9bAIBMAcGBWeBDAEB
MAgGBmeBDAECATAIBgZngQwBAgIwCAYGZ4EMAQIDMA0GCSqGSIb3DQEBCwUAA4IB
AQCAMs5eC91uWg0Kr+HWhMvAjvqFcO3aXbMM9yt1QP6FCvrzMXi3cEsaiVi6gL3z
ax3pfs8LulicWdSQ0/1s/dCYbbdxglvPbQtaCdB73sRD2Cqk3p5BJl+7j5nL3a7h
qG+fh/50tx8bIKuxT8b1Z11dmzzp/2n3YWzW2fP9NsarA4h20ksudYbj/NhVfSbC
EXffPgK2fPOre3qGNm+499iTcc+G33Mw+nur7SpZyEKEOxEXGlLzyQ4UfaJbcme6
ce1XR2bFuAJKZTRei9AqPCCcUZlM51Ke92sRKw2Sfh3oius2FkOH6ipjv3U/697E
A7sKPPcw7+uvTPyLNhBzPvOk
-----END CERTIFICATE-----
)ROOT_CA";

const char* public_key = R"PUBLIC_KEY(
  -----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAqYEgW6K3LVu7v+uaJJRL
gC7z8xQmy4CPBSX2W9Qe5NlpO0XQ4RnuIX0X9nfKEhITeB0EZ5R59rVPIIWnbHRk
qBE5lXnf7YSbFDh9ZT7am080TCF45REPnCnDeOiYbY1q9+/nONpTHCkC1bffHeIn
D0S9r/BjL+0I4fXyhm9GL9Q2BEEsWT7ufzsIP3ADllCvB/XoKKNIXsvFECCkJrPH
OdtMKR+6K6eNAKDT1IOlFlbdyGE5t+kGRguIh0LhuBp0trNEdEaGUxYE2UN/V5YX
mw0czPWcdyCCtbwx0xv8NiImZ3ZhE8O17sdWjqPtBYx9yKDNdH/IXNGY05fohXbk
ZrYsn5f4oIuiAObw+Xl+9bSgaRqJ0ezoegF8uIFpOUtNhZ3hygAuiPwbCCXn94Dk
y2d1LPGvaHae5t8OOMZ7dT7GCV6cJ3RhyKaZfoNklLKly9ab0oTbwmthLspX3wGj
MvTWw5vSCXHWh8xLE2LH15rpcBkh7SYmdoLu9xMEAcD+GYU9DmTcDSUEgfHnSMff
66u0lAJzUwJBZ1sV8Ec1j08cuIEUuV7AtHvV9CsSQtyXVxmGF1UVk8SCUuOYTma6
L1G9vkoVljfd+/VWbSJR67T0BMcapzJD3Vc/rY6iBT4lB1J7wum6oyH9K6frl/zP
1Owwdkc0YGEFCiDjUPSvuuECAwEAAQ==
-----END PUBLIC KEY-----
)PUBLIC_KEY";

CryptoMemAsset *MyRootCA = new CryptoMemAsset("Root CA", root_ca, strlen(root_ca)+1);
CryptoMemAsset *MyPubKey = new CryptoMemAsset("RSA Key", public_key, strlen(public_key)+1);

bool doOTAUpdate = false;
int ota_current_partition = U_SPIFFS;
unsigned long ota_last_message = 0;

//for making a captive portal
const byte DNS_PORT = 53;
IPAddress apIP(8,8,4,4); // The default android DNS
DNSServer dnsServer;

//default config info for our wifi
String wifi_ssid = "Yarrboard";
String wifi_pass = "";
String wifi_mode = "ap";

//username / password for websocket authentication
String app_user = "admin";
String app_pass = "admin";
bool require_login = true;
String local_hostname = "yarrboard";

//keep track of our channel info.
const byte channelCount = 8;
const byte outputPins[channelCount] = { 32, 33, 25, 26, 27, 14, 12, 13 };

//for watching our power supply
float busVoltage = 0;

//state information for all our channels.
bool channelState[channelCount];
bool channelTripped[channelCount];
float channelDutyCycle[channelCount];
bool channelIsEnabled[channelCount];
bool channelIsDimmable[channelCount];
unsigned long channelLastDutyCycleUpdate[channelCount];
bool channelDutyCycleIsThrottled[channelCount];
float channelAmperage[channelCount];
float channelSoftFuseAmperage[channelCount];
float channelAmpHour[channelCount];
float channelWattHour[channelCount];
String channelNames[channelCount];
unsigned int channelStateChangeCount[channelCount];
unsigned int channelSoftFuseTripCount[channelCount];

//keep track of our authenticated clients
const byte clientLimit = 8;
uint32_t authenticatedClientIDs[clientLimit];

/* Setting PWM Properties */
const int PWMFreq = 5000; /* in Hz  */
const int PWMResolution = 8;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

//storage for more permanent stuff.
Preferences preferences;

//our server variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//for tracking our ADC loop
int adcInterval = 100;
unsigned long previousADCMillis = 0;

//for tracking our message loop
int messageInterval = 250;
unsigned long previousMessageMillis = 0;
unsigned int handledMessages = 0;
unsigned int lastHandledMessages = 0;
unsigned long totalHandledMessages = 0;

//time variables
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
struct tm timeinfo;

//
//  Function definitions
//
void setupWifi();
bool connectToWifi(String ssid, String pass);

void otaUpdateSetup();
void sendOTAProgressUpdate(float progress, int partition);
void sendOTAProgressFinished();

void timeAvailable(struct timeval *t);
void printLocalTime();

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
void handleReceivedMessage(char *payload, AsyncWebSocketClient *client);

bool assertLoggedIn(AsyncWebSocketClient *client);
bool assertValidChannel(byte cid, AsyncWebSocketClient *client);

void sendToAll(String jsonString);
void sendUpdate();
void sendConfigJSON(AsyncWebSocketClient *client);
void sendNetworkConfigJSON(AsyncWebSocketClient *client);
void sendStatsJSON(AsyncWebSocketClient *client);
void sendSuccessJSON(String success, AsyncWebSocketClient *client);
void sendErrorJSON(String error, AsyncWebSocketClient *client);

void updateChannelState(int channelId);
void checkSoftFuses();

void setup()
{
  unsigned long setup_t1 = micros();
  String prefIndex;

  //startup our serial
  Serial.begin(115200);
  delay(10);
  Serial.print("Yarrboard ");
  Serial.println(VERSION);

  //Setup our NTP to get the current time.
  //sntp_set_time_sync_notification_cb(timeAvailable);
  sntp_servermode_dhcp(1);  // (optional)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  //really nice library for permanently storing preferences.
  preferences.begin("yarrboard", false);

  //intitialize our output pins.
  for (short i = 0; i < channelCount; i++)
  {
    //init our storage variables.
    channelState[i] = false;
    channelTripped[i] = false;
    channelAmperage[i] = 0.0;
    channelAmpHour[i] = 0.0;
    channelWattHour[i] = 0.0;
    channelLastDutyCycleUpdate[i] = 0;
    channelDutyCycleIsThrottled[i] = false;

    //initialize our PWM channels
    pinMode(outputPins[i], OUTPUT);
    analogWrite(outputPins[i], 0);

    //lookup our name
    prefIndex = "cName" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelNames[i] = preferences.getString(prefIndex.c_str());
    else
      channelNames[i] = "Channel #" + String(i);

    //enabled or no
    prefIndex = "cEnabled" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelIsEnabled[i] = preferences.getBool(prefIndex.c_str());
    else
      channelIsEnabled[i] = true;

    //lookup our duty cycle
    prefIndex = "cDuty" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelDutyCycle[i] = preferences.getFloat(prefIndex.c_str());
    else
      channelDutyCycle[i] = 1.0;

    //dimmability.
    prefIndex = "cDimmable" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelIsDimmable[i] = preferences.getBool(prefIndex.c_str());
    else
      channelIsDimmable[i] = true;

    //soft fuse
    prefIndex = "cSoftFuse" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelSoftFuseAmperage[i] = preferences.getFloat(prefIndex.c_str());
    else
      channelSoftFuseAmperage[i] = 20.0;

    //soft fuse trip count
    prefIndex = "cTripCount" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelSoftFuseTripCount[i] = preferences.getUInt(prefIndex.c_str());
    else
      channelSoftFuseTripCount[i] = 0;
  }

  //look up our board name
  if (preferences.isKey("boardName"))
    board_name = preferences.getString("boardName");

  //wifi login info.
  if (preferences.isKey("wifi_mode"))
  {
    is_first_boot = false;
    wifi_mode = preferences.getString("wifi_mode");
    wifi_ssid = preferences.getString("wifi_ssid");
    wifi_pass = preferences.getString("wifi_pass");
  }
  else
    is_first_boot = true;

  //look up our username/password
  if (preferences.isKey("app_user"))
    app_user = preferences.getString("app_user");
  if (preferences.isKey("app_pass"))
    app_pass = preferences.getString("app_pass");
  if (preferences.isKey("require_login"))
    require_login = preferences.getBool("require_login");
  if (preferences.isKey("local_hostname"))
    local_hostname = preferences.getString("local_hostname");

  //our analog stuff
  adc_setup();

  //get a unique ID for us
  byte mac[6];
  WiFi.macAddress(mac);
  uuid = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  Serial.print("UUID: ");
  Serial.println(uuid);

  //get an IP address
  setupWifi();

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //checking versions
  otaUpdateSetup();

  //config for our websocket server
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  //we are only serving static files - big cache
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=2592000");
  server.begin();

  unsigned long setup_t2 = micros();
  Serial.print("Boot time: ");
  Serial.print((float)(setup_t2 - setup_t1) / 1000.0);
  Serial.println("ms");
}

void otaUpdateSetup()
{
  esp32FOTA.setManifestURL(manifest_url);
  esp32FOTA.setRootCA(MyRootCA);
  //esp32FOTA.setPubKey(MyPubKey);

  esp32FOTA.setUpdateBeginFailCb( [](int partition) {
    Serial.printf("[ota] Update could not begin with %s partition\n", partition==U_SPIFFS ? "spiffs" : "firmware" );
  });

  // usage with lambda function:
  esp32FOTA.setProgressCb( [](size_t progress, size_t size)
  {
      if( progress == size || progress == 0 )
        Serial.println();
      Serial.print(".");

      //let the clients know every second and at the end
      if (millis() - ota_last_message > 1000 || progress == size)
      {
        float percent = (float)progress / (float)size * 100.0;
        sendOTAProgressUpdate(percent, ota_current_partition);
        ota_last_message = millis();
      }
  });

  esp32FOTA.setUpdateEndCb( [](int partition) {
    Serial.printf("[ota] Update ended with %s partition\n", partition==U_SPIFFS ? "spiffs" : "firmware" );

    //no begin callback???
    if (partition == U_SPIFFS)
      ota_current_partition = U_FLASH;
    //i guess we're done?
    else
      sendOTAProgressFinished();
  });

  esp32FOTA.setUpdateCheckFailCb( [](int partition, int error_code) {
    Serial.printf("[ota] Update could not validate %s partition (error %d)\n", partition==U_SPIFFS ? "spiffs" : "firmware", error_code );
    // error codes:
    //  -1 : partition not found
    //  -2 : validation (signature check) failed
  });

  //esp32FOTA.printConfig();
}

// Callback function (get's called when time adjusts via NTP)
void timeAvailable(struct timeval *t) {
  Serial.print("NTP update: ");
  printLocalTime();
}

void loop()
{
  unsigned long t1;
  unsigned long t2;

  //sometimes websocket clients die badly.
  ws.cleanupClients();

  //do we want to do the update?
  if (doOTAUpdate)
  {
    esp32FOTA.handle();
    return;
  }

  //run our dns... for AP mode
  if (wifi_mode.equals("ap"))
    dnsServer.processNextRequest();

  //run our ADC on a faster loop
  int adcDelta = millis() - previousADCMillis;
  if (adcDelta >= adcInterval)
  {
    //this is a bit slow, so only do it once per update
    for (byte channel = 0; channel < channelCount; channel++)
      channelAmperage[channel] = adc_readAmperage(channel);

    //check what our power is.
    busVoltage = adc_readBusVoltage();
  
    //record our total consumption
    for (byte i = 0; i < channelCount; i++) {
      if (channelAmperage[i] > 0)
      {
        channelAmpHour[i] += channelAmperage[i] * ((float)adcDelta / 3600000.0);
        channelWattHour[i] += channelAmperage[i] * busVoltage * ((float)adcDelta / 3600000.0);
      }
    }

    //are our loads ok?
    checkSoftFuses();

    // Use the snapshot to set track time until next event
    previousADCMillis = millis();
  }

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

  //are any of our channels throttled?
  for (byte id = 0; id < channelCount; id++)
  {
    //after 5 secs of no activity, we can save it.
    if (channelDutyCycleIsThrottled[id] && millis() - channelLastDutyCycleUpdate[id] > 5000)
    {
      String prefIndex = "cDuty" + String(id);
      preferences.putFloat(prefIndex.c_str(), channelDutyCycle[id]);
      channelDutyCycleIsThrottled[id] = false;
    }
  }
}

void printLocalTime()
{
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char buffer[40];
  strftime(buffer, 40, "%FT%T%z", &timeinfo);
  Serial.println(buffer);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
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
    bool updatedNeeded = esp32FOTA.execHTTPcheck();
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
  if (cid < 0 || cid >= channelCount) {
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
  object["version"] = VERSION;
  object["name"] = board_name;
  object["uuid"] = uuid;
  object["msg"] = "config";

  //do we want to flag it for config?
  if (is_first_boot)
    object["first_boot"] = true;

  //send our configuration
  for (byte i = 0; i < channelCount; i++) {
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
  for (byte i = 0; i < channelCount; i++) {
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

void sendOTAProgressUpdate(float progress, int partition)
{
  StaticJsonDocument<96> doc;
  String jsonString;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["msg"] = "ota_progress";

  if (partition == U_SPIFFS)
    object["partition"] = "spiffs";
  else
    object["partition"] = "firmware";

  object["progress"] = round2(progress);

  //send it.
  serializeJson(doc, jsonString);
  sendToAll(jsonString);
}

void sendOTAProgressFinished()
{
  String jsonString = "{\"msg\":\"ota_finished\"}";
  sendToAll(jsonString);
}

void sendUpdate()
{
  StaticJsonDocument<2048> doc;
  String jsonString;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["msg"] = "update";
  object["bus_voltage"] = busVoltage;

  if (getLocalTime(&timeinfo)) {
    char buffer[80];
    strftime(buffer, 80, "%FT%T%z", &timeinfo);
    object["time"] = buffer;
  }

  for (byte i = 0; i < channelCount; i++) {
    object["channels"][i]["id"] = i;
    object["channels"][i]["state"] = channelState[i];
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

void setupWifi()
{
  //some global config
  WiFi.setSleep(false);
  WiFi.setHostname(local_hostname.c_str());
  WiFi.useStaticBuffers(true);  //from: https://github.com/espressif/arduino-esp32/issues/7183
  WiFi.mode(WIFI_AP_STA);

  Serial.print("Hostname: ");
  Serial.print(local_hostname);
  Serial.println(".local");

  //which mode do we want?
  if (wifi_mode.equals("client"))
  {
    Serial.print("Client mode: ");
    Serial.print(wifi_ssid);
    Serial.print(" / ");
    Serial.println(wifi_pass);

    //try and connect
    connectToWifi(wifi_ssid, wifi_pass);
  }
  //default to AP mode.
  else
  {
    Serial.print("AP mode: ");
    Serial.print(wifi_ssid);
    Serial.print(" / ");
    Serial.println(wifi_pass);

    WiFi.softAP(wifi_ssid, wifi_pass);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    Serial.print("AP IP address: ");
    Serial.println(apIP);

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);
  }

  //setup our local name.
  if (!MDNS.begin(local_hostname)) {
    Serial.println("Error starting mDNS");
    return;
  }
  MDNS.addService("http", "tcp", 80);
}

bool connectToWifi(String ssid, String pass)
{
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());

  // How long to try for?
  int tryDelay = 500;
  int numberOfTries = 60;

  // Wait for the WiFi event
  while (true)
  {
    switch (WiFi.status())
    {
      case WL_NO_SSID_AVAIL:
        Serial.println("[WiFi] SSID not found");
        return false;
        break;
      case WL_CONNECT_FAILED:
        Serial.print("[WiFi] Failed");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("[WiFi] Connection was lost");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("[WiFi] Scan is completed");
        break;
      case WL_DISCONNECTED:
        Serial.println("[WiFi] WiFi is disconnected");
        break;
      case WL_CONNECTED:
        Serial.println("[WiFi] WiFi is connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());
        return true;
        break;
      default:
        Serial.print("[WiFi] WiFi Status: ");
        Serial.println(WiFi.status());
        break;
    }

    //have we hit our limit?
    if(numberOfTries <= 0)
    {
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return false;
    } else {
      numberOfTries--;
    }

    delay(tryDelay);
  }

  return false;
}

void updateChannelState(int channelId)
{
  //what PWM do we want?
  int pwm = 0;
  if (channelIsDimmable[channelId])
    pwm = (int)(channelDutyCycle[channelId] * MAX_DUTY_CYCLE);
  else
    pwm = MAX_DUTY_CYCLE;

  //if its tripped, zero it out.
  if (channelTripped[channelId])
    pwm = 0;

  //okay, set our pin state.
  if (channelState[channelId])
    analogWrite(outputPins[channelId], pwm);
  else
    analogWrite(outputPins[channelId], 0);
}

void checkSoftFuses()
{
  for (byte channel = 0; channel < channelCount; channel++)
  {
    //only trip once....
    if (!channelTripped[channel])
    {
      //Check our soft fuse, and our max limit for the board.
      if (abs(channelAmperage[channel]) >= channelSoftFuseAmperage[channel] || abs(channelAmperage[channel]) >= 20.0)
      {
        //record some variables
        channelTripped[channel] = true;
        channelState[channel] = false;
        channelSoftFuseTripCount[channel]++;

        //save to our storage
        String prefIndex = "cTripCount" + String(channel);
        preferences.putUInt(prefIndex.c_str(), channelSoftFuseTripCount[channel]);

        //actually shut it down!
        updateChannelState(channel);
      }
    }
  }
}