/*
  Gnarboard v1
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/Gnarboard
  License: GPLv3

*/

#include <WiFi.h>               // Standard library
#include <Preferences.h>        // Standard library
//#include <SPIFFS.h>             // Standard library
#include <ArduinoJson.h>        // ArduinoJSON by Benoit Blanchon via library manager
#include <MCP3208.h>            // MPC3208 by Rodolvo Prieto via library manager
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer/ via .zip
#include <AsyncTCP.h>           // https://github.com/me-no-dev/AsyncTCP/ via .zip
//#include <NMEA2000.h>           // https://github.com/ttlappalainen/NMEA2000_esp32 via .zip
//#include <NMEA2000_CAN.h>       // https://github.com/ttlappalainen/NMEA2000 via .zip
//#include <N2kMsg.h>             // same ^^^
//#include <N2kDeviceList.h>      // same ^^^

//identify yourself!
const char* version = "Gnarboard v1.0.0";
String uuid;

//ke ep track of our channel info.
const byte channelCount = 8;
const byte outputPins[channelCount] = {25, 26, 27, 14, 12, 13, 17, 16};
const byte analogPins[channelCount] = {36, 39, 34, 35, 32, 33, 4, 2};

//state information for all our channels.`  1 
bool   channelState[channelCount];
bool   channelTripped[channelCount];
float  channelDutyCycle[channelCount];
bool   channelSaveDutyCycle[channelCount];
float  channelAmperage[channelCount];
float  channelSoftFuseAmperage[channelCount];
float  channelAmpHour[channelCount];
String channelNames[channelCount];

/* Setting PWM Properties */
const int PWMFreq = 5000; /* in Hz  */
const int PWMResolution = 8;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

//object for our adc
MCP3208 adc;

//storage for more permanent stuff.
Preferences preferences;

//wifi login info.
String ssid;
String password;

//our server variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//for tracking our loop
int adcInterval = 100;     // virtual delay
unsigned long previousADCMillis = 0; // Tracks the time since last event fired

int messageInterval = 250; // virtual delay
unsigned long previousMessageMillis = 0; // Tracks the time since last event fired
unsigned int handledMessages = 0;
unsigned int lastHandledMessages = 0;
unsigned long totalHandledMessages = 0;

/*
//NMEA2000 stuff
int NodeAddress = 0;  // To store last Node Address
const unsigned long TransmitMessages[] PROGMEM = {126208UL,   // Set Pilot Mode
                                                  126720UL,   // Send Key Command
                                                  65288UL,    // Send Seatalk Alarm State
                                                  0
                                                 };

const unsigned long ReceiveMessages[] PROGMEM = { 127250UL,   // Read Heading
                                                  65288UL,    // Read Seatalk Alarm State
                                                  65379UL,    // Read Pilot Mode
                                                  0
                                                };

tN2kDeviceList *pN2kDeviceList;
short pilotSourceAddress = -1;
*/

/*
// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}
*/

void setup()
{
  //startup our serial
  Serial.begin(115200);
  delay(10);
  Serial.println(version);

  //where our websites are stored
  //initSPIFFS();

  //for storing stuff to flash
  preferences.begin("gnarboard", false);

  //intitialize our output pins.
  for (byte i=0; i<channelCount; i++)
  {
    //init our storage variables.
    channelState[i] = false;
    channelDutyCycle[i] = 0.0;
    channelTripped[i] = false;
    channelAmpHour[i] = 0.0;
    channelAmperage[i] = 0.0;
    channelSoftFuseAmperage[i] = 20.0;
    channelSaveDutyCycle[i] = true;

    //initialize our PWM channels
    //ledcSetup(i, PWMFreq, PWMResolution);
    //ledcAttachPin(outputPins[i], i);
    //ledcWrite(i, 0);

    //initialize our PWM channels
    pinMode(outputPins[i], OUTPUT);
    analogWrite(outputPins[i], 0);

    //lookup our name
    String prefIndex = "channel_name_" + i;
    channelNames[i] = preferences.getString(prefIndex.c_str());
  }

  //setup our adc
  setupADC();

  //load our saved duty cycle data
  preferences.getBytes("duty_cycle", &channelDutyCycle, sizeof(channelDutyCycle));  
  preferences.getBytes("soft_fuse", &channelSoftFuseAmperage, sizeof(channelSoftFuseAmperage));  

  //get a unique ID for us
  byte mac[6];
  WiFi.macAddress(mac);
  uuid = String(mac[0],HEX) +String(mac[1],HEX) +String(mac[2],HEX) +String(mac[3],HEX) + String(mac[4],HEX) + String(mac[5],HEX);
  Serial.println(uuid);

  //get an IP address
  connectToWifi();

  // Start server
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();

  //setupNMEA2000();
}

void setupNMEA2000()
{
  /*
  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega
  NMEA2000.SetN2kCANReceiveFrameBufSize(150);
  NMEA2000.SetN2kCANMsgBufSize(8);
  
  // Set Product information
  NMEA2000.SetProductInformation("00000001", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 "Evo Pilot Remote",  // Manufacturer's Model ID
                                 "1.0.0.0",  // Manufacturer's Software version code
                                 "1.0.0.0" // Manufacturer's Model version
                                );
  
  // Set device information
  NMEA2000.SetDeviceInformation(0, // Unique number. Use e.g. Serial number.
                                132, // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );

  
  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, NodeAddress); //N2km_NodeOnly N2km_ListenAndNode
  NMEA2000.ExtendTransmitMessages(TransmitMessages);
  NMEA2000.ExtendReceiveMessages(ReceiveMessages);

  //NMEA2000.SetMsgHandler(RaymarinePilot::HandleNMEA2000Msg);

    pN2kDeviceList = new tN2kDeviceList(&NMEA2000);
  //NMEA2000.SetDebugMode(tNMEA2000::dm_ClearText); // Uncomment this, so you can test code without CAN bus chips on Arduino Mega
  NMEA2000.EnableForward(false); // Disable all msg forwarding to USB (=Serial)
  NMEA2000.Open();
  */
}

void setupADC()
{
  setupMCP3208();
}

unsigned long t1;
unsigned long t2;

void loop()
{
  // websocket server method that handles all clients
  ws.cleanupClients();

  //run our ADC on a faster loop
  int adcDelta = millis() - previousADCMillis;
  if (adcDelta >= adcInterval)
  {
    //this is a bit slow, so only do it once per update

    //t1 = micros();
    readAmperages();
    //t2 = micros();
    //Serial.print("ADC: ");
    //Serial.print((float)(t2 - t1) / 1000.0);
    //Serial.println("ms");

    //record our total consumption
    for (byte i=0; i<channelCount; i++)
      channelAmpHour[i] += channelAmperage[i] * ((float)adcDelta / 3600000.0);

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
    t1 = micros();
    sendUpdate();
    t2 = micros();
    //Serial.print("Send Message: ");
    //Serial.print((float)(t2 - t1) / 1000.0);
    //Serial.println("ms");

    //how fast are we?
    Serial.print(messageDelta);
    Serial.print("ms | msg/s: ");
    Serial.print(handledMessages - lastHandledMessages);
    Serial.println();

    //Serial.print("Heap: ");
    //Serial.println(ESP.getFreeHeap());

    //Serial.print("RRSI: ");
    //Serial.println(WiFi.RSSI());

    //for keeping track.
    lastHandledMessages = handledMessages;
    previousMessageMillis = millis();
  }

  /*
  unsigned long currentMillis = millis();
// if WiFi is down, try reconnecting
if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
  Serial.print(millis());
  Serial.println("Reconnecting to WiFi...");
  WiFi.disconnect();
  WiFi.reconnect();
  previousMillis = currentMillis;
}
*/
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      sendBoardInfo();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    //Serial.printf("[WebSocket] Message: %s\n", data);
    handleReceivedMessage((char*)data);
  }
}

void handleReceivedMessage(char *payload) {
  StaticJsonDocument<2048> doc;
  deserializeJson(doc, payload);
  //JsonObject root = doc.as<JsonObject>();

  //what is your command?
  String cmd = doc["cmd"];

  //change state?
  if (cmd.equals("state"))
  {
    //is it a valid channel?
    byte cid = doc["id"];
    if (cid < 0 || cid >= channelCount)
    {
      sendError("Invalid ID");
      return;
    }

    //what is our new state?
    bool state = doc["value"];
    channelState[cid] = state;

    //reset soft fuse when we turn on
    if (state)
      channelTripped[cid] = false;

    //change our output pin to reflect
    updateChannelState(cid);
  }
  //change duty cycle?
  else if (cmd.equals("duty"))
  {
    //is it a valid channel?
    byte cid = doc["id"];
    if (cid < 0 || cid >= channelCount)
    {
      sendError("Invalid ID");
      return;
    }

    float value = doc["value"];
    
    if (value < 0)
      sendError("Duty cycle must be >= 0");
    else if (value > 20)
      sendError("Duty cycle must be <= 1");
    else
    {
      channelDutyCycle[cid] = value;

      //do we save our saved duty cycle data?
      if (channelSaveDutyCycle[cid])
      {
        Serial.println("Saving");
        preferences.putBytes("duty_cycle", &channelDutyCycle, sizeof(channelDutyCycle));
      }
    }

    //change our output pin to reflect
    updateChannelState(cid);
  }
  //change a soft fuse setting?
  else if(cmd.equals("soft_fuse"))
  {
    //is it a valid channel?
    byte cid = doc["id"];
    if (cid < 0 || cid >= channelCount)
    {
      sendError("Invalid ID");
      return;
    }

    float value = doc["value"];
    if (value < 0)
      sendError("Soft fuse minimum is 0 amps.");
    else if (value > 20)
      sendError("Soft fuse maximum is 20 amps.");
    else
    {
      channelSoftFuseAmperage[cid] = value;

      //save our saved duty cycle data
      preferences.putBytes("soft_fuse", &channelSoftFuseAmperage, sizeof(channelSoftFuseAmperage));  
    }
  }
  //save our duty cycle?
  else if(cmd.equals("save_duty_cycle"))
  {
    //is it a valid channel?
    byte cid = doc["id"];
    if (cid < 0 || cid >= channelCount)
    {
      sendError("Invalid ID");
      return;
    }

    bool value = doc["value"];
    channelSaveDutyCycle[cid] = value;
  }
  //change a channel name?
  else if(cmd.equals("set_name"))
  {
    //is it a valid channel?
    byte cid = doc["id"];
    if (cid < 0 || cid >= channelCount)
    {
      sendError("Invalid ID");
      return;
    }

    String value = doc["value"];
    if (value.length() > 64)
      sendError("Maximum channel name length is 64 characters.");
    else
    {
      channelNames[cid] = value;

      //save to our storage
      String prefIndex = "channel_name_" + cid;
      preferences.putString(prefIndex.c_str(), value);
    }
  }
  //get our config?
  else if(cmd.equals("config"))
  {
    sendBoardInfo();
  }
  //wrong command.
  else
  {
      sendError("Invalid command.");
  }

  //keep track!
  handledMessages++;
  totalHandledMessages++;
}

void sendError(String error)
{
  String jsonString; // Temporary storage for the JSON String

  Serial.print("Error: ");
  Serial.println(error);

  StaticJsonDocument<1000> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["error"] = error;

  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString); 
  //Serial.println( jsonString );
  ws.textAll(jsonString);
}

void sendBoardInfo()
{
  String jsonString;
  StaticJsonDocument<5000> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  //our identifying info
  object["name"] = version;
  object["uuid"] = uuid;
  object["msg"] = "config";
  object["totalMessages"] = totalHandledMessages;

  //send our configuration
  for (byte i=0; i<channelCount; i++)
  {
    object["channels"][i]["id"] = i;
    object["channels"][i]["name"] = channelNames[i];
    object["channels"][i]["type"] = "mosfet";
    object["channels"][i]["hasPWM"] = true;
    object["channels"][i]["hasCurrent"] = true;
    object["channels"][i]["softFuse"] = round2(channelSoftFuseAmperage[i]);
    object["channels"][i]["saveDutyCycle"] = channelSaveDutyCycle[i];
  }
  
  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString); 
  
  //Serial.println( jsonString );

  // send the JSON object through the websocket
  ws.textAll(jsonString);
}

void sendUpdate()
{
  StaticJsonDocument<5000> doc;
  String jsonString;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["msg"] = "update";

  for (byte i=0; i<channelCount; i++)
  {
    object["channels"][i]["id"] = i;
    object["channels"][i]["state"] = channelState[i];
    object["channels"][i]["duty"] = round2(channelDutyCycle[i]);
    object["channels"][i]["current"] = round2(channelAmperage[i]);
    object["channels"][i]["aH"] = round3(channelAmpHour[i]);

    if (channelTripped[i])
      object["channels"][i]["soft_fuse_tripped"] = true;
  }

  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString); 
  //Serial.println( jsonString );
  ws.textAll(jsonString);
}

double round2(double value) {
   return (long)(value * 100 + 0.5) / 100.0;
}

double round3(double value) {
   return (long)(value * 1000 + 0.5) / 1000.0;
}

double round4(double value) {
   return (long)(value * 10000 + 0.5) / 10000.0;
}


void connectToWifi()
{
  ssid = preferences.getString("ssid", "Wind.Ninja"); 
  password = preferences.getString("password", "chickenloop");

  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.setSleep(false);
  WiFi.setHostname("gnarboard");
  WiFi.useStaticBuffers(true); //from: https://github.com/espressif/arduino-esp32/issues/7183
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // How long to try
  int tryDelay = 500;
  int numberOfTries = 1000;

  // Wait for the WiFi event
  while (true) {
      
      switch(WiFi.status()) {
        case WL_NO_SSID_AVAIL:
          Serial.println("[WiFi] SSID not found");
          break;
        case WL_CONNECT_FAILED:
          Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
          return;
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
          return;
          break;
        default:
          Serial.print("[WiFi] WiFi Status: ");
          Serial.println(WiFi.status());
          break;
      }
      delay(tryDelay);

      /*      
      if(numberOfTries <= 0){
        Serial.print(".");
        // Use disconnect function to force stop trying to connect
        WiFi.disconnect();
        return;
      } else {
        numberOfTries--;
      }
      */
  }
}

void updateChannelState(int channelId)
{
  //it has to be set to on and soft fuse not tripped.
  if (channelState[channelId] && !channelTripped[channelId])
  {
    int pwm = (int)(channelDutyCycle[channelId] * MAX_DUTY_CYCLE);
    //ledcWrite(channelId, pwm);
    analogWrite(outputPins[channelId], pwm);
  }
  else
  {
    //ledcWrite(outputPins[channelId], pow(2, PWMResolution));
    analogWrite(outputPins[channelId], 0);
  }
}

void setupMCP3208()
{
  adc.begin();
}

uint16_t readMCP3208Channel(byte channel, byte samples = 64)
{
  uint32_t value = 0;

  if (samples > 1)
  {
    for (byte i=0; i<samples; i++)
    {
      value += adc.readADC(channel);
    }
    value = value / samples;
  }
  else
    value = adc.readADC(channel);

  return (uint16_t)value;
}

void readAmperages()
{
  for (byte channel = 0 ; channel < channelCount; channel++)
  {
    uint16_t val = readMCP3208Channel(channel);

    float volts = val * (3.3 / 4096.0);
    
    float amps = 0.0;
    //volts = volts / 0.6875;
    //amps = (2.5 - volts) / (0.100); //ACS712 5V w/ voltage divider
    amps = (volts - (3.3 * 0.1)) / (0.200); //TMCS1108A3U
    //amps = (volts - (3.3 * 0.1)) / (0.100); //TMCS1108A2U
    //amps = (volts - (3.3 * 0.1)) / (0.132); //ACS725LLCTR-20AU
    //amps = (volts - (3.3 * 0.5)) / (0.066);  //MCS1802-20
    //amps = (volts - 0.650) / (0.100);       //CT427-xSN820DR
    
    //Serial.print(channel);
    //Serial.print(" CH | ");
    //Serial.print(val);
    //Serial.print(" ADC | ");
    //Serial.print(volts);
    //Serial.print(" V | ");
    //Serial.print(amps);
    //Serial.print(" A | ");

    channelAmperage[channel] = amps;
  }
  //Serial.println();
}


void checkSoftFuses()
{
  for (byte channel = 0 ; channel < channelCount; channel++)
  {
    if (abs(channelAmperage[channel]) >= channelSoftFuseAmperage[channel])
      channelTripped[channel] = true;
    else if (abs(channelAmperage[channel]) >= 20.0)
      channelTripped[channel] = true;
  }
}