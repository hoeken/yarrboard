/*
  Gnarboard v1
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/Gnarboard
  License: GPLv3

*/

#include <WiFi.h>               // Standard library
#include <Preferences.h>        // Standard library
#include <SPIFFS.h>             // Standard library
#include <ArduinoJson.h>        // ArduinoJSON by Benoit Blanchon via library manager
#include <WebSocketsServer.h>   // WebSockets by Markus Sattler via library manager
#include <MCP3208.h>            // MPC3208 by Rodolvo Prieto via library manager
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer/ via .zip
#include <AsyncTCP.h>           // https://github.com/me-no-dev/AsyncTCP/ via .zip
#include <NMEA2000.h>           // https://github.com/ttlappalainen/NMEA2000_esp32 via .zip
#include <NMEA2000_CAN.h>       // https://github.com/ttlappalainen/NMEA2000 via .zip
#include <N2kMsg.h>             // same ^^^
#include <N2kDeviceList.h>      // same ^^^

//identify yourself!
const char* version = "Gnarboard v1.0.0";
String uuid;

//keep track of our channel info.
const byte channelCount = 8;
const byte outputPins[channelCount] = {25, 26, 27, 14, 12, 13, 17, 16};
const byte analogPins[channelCount] = {36, 39, 34, 35, 32, 33, 4, 2};

//state information for all our channels.`  1 
bool   channelState[channelCount] = {false, false, false, false, false, false, false, false};
float  channelDutyCycle[channelCount] = {0, 0, 0, 0, 0, 0, 0, 0};
float  channelAmperage[channelCount];
float  channelSoftFuseAmperage[channelCount];
String channelNames[channelCount];

/* Setting PWM Properties */
const int PWMFreq = 5000; /* in Hz  */
const int PWMResolution = 8;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

//storage for more permanent stuff.
Preferences preferences;

//wifi login info.
String ssid;
String password;

//our server variable
WebSocketsServer webSocket = WebSocketsServer(8080);  //create instance for webSocket server
String jsonString; // Temporary storage for the JSON String

//for tracking our loop
int interval = 1000; // virtual delay
unsigned long previousMillis = 0; // Tracks the time since last event fired
unsigned int handledMessages = 0;
unsigned int lastHandledMessages = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded
// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

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

void setup()
{
  //startup our serial
  Serial.begin(115200);
  delay(10);
  Serial.println(version);

  //where our websites are stored
  initSPIFFS();

  //for storing stuff to flash
  preferences.begin("gnarboard", false);

  //intitialize our output pins.
  for (byte i=0; i<channelCount; i++)
  {
    //initialize our PWM channels
    //ledcSetup(i, PWMFreq, PWMResolution);
    //ledcAttachPin(outputPins[i], i);
    //ledcWrite(i, 0);

    pinMode(outputPins[i], OUTPUT);
    analogWrite(outputPins[i], 0);

    //just init some values, will get real ones later.
    channelAmperage[i] = 0.0;
    //channelSoftFuseAmperage[i] = 20;

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

  //start our websocket server.
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

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
}

void setupADC()
{
  setupMCP3208();
}

void loop()
{
  // websocket server method that handles all clients
  webSocket.loop(); 

  //lookup our info periodically  
  unsigned long currentMillis = millis();
  unsigned long deltaMillis = (currentMillis - previousMillis);
  if (deltaMillis >= interval)
  {
    //this is a bit slow, so only do it once per update
    readAmperages();

    //read and send out our json update
    sendUpdate();

    // Use the snapshot to set track time until next event
    previousMillis = currentMillis;   

    //how fast are we?
    Serial.print("delta: ");
    Serial.print(deltaMillis);
    Serial.print(" | msg/s: ");
    Serial.print(handledMessages - lastHandledMessages);
    Serial.println();

    //for keeping track.
    lastHandledMessages = handledMessages;
  }
}

// This function gets a call when a WebSocket event occurs
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: // enum that read status this is used for debugging.
      Serial.println("[WebSocket] Disconnected");
      break;
    case WStype_CONNECTED:  // Check if a WebSocket client is connected or not
      Serial.println("[WebSocket] Connected");
      sendBoardInfo();
      break;
    case WStype_TEXT: // check response from client
      //Serial.printf("[WebSocket] Message: %s\n", payload);
      handleReceivedMessage((char*)payload);
      break;
  }
}

void handleReceivedMessage(char *payload) {
  StaticJsonDocument<200> doc;
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

      //save our saved duty cycle data
      preferences.putBytes("duty_cycle", &channelDutyCycle, sizeof(channelDutyCycle));
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
}

void sendError(String error)
{
  Serial.print("Error: ");
  Serial.println(error);

  StaticJsonDocument<500> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["error"] = error;

  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString); 
  //Serial.println( jsonString );
  webSocket.broadcastTXT(jsonString); 
  jsonString = ""; // clear the String.
}

void sendBoardInfo()
{
  StaticJsonDocument<2048> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  //our identifying info
  object["name"] = version;
  object["uuid"] = uuid;
  object["msg"] = "config";

  //send our configuration
  for (byte i=0; i<channelCount; i++)
  {
    object["loads"][i]["id"] = i;
    object["loads"][i]["name"] = channelNames[i];
    object["loads"][i]["type"] = "mosfet";
    object["loads"][i]["hasPWM"] = true;
    object["loads"][i]["hasCurrent"] = true;
    object["loads"][i]["softFuse"] = channelSoftFuseAmperage[i];
  }
  
  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString); 
  
  //Serial.println( jsonString );

  // send the JSON object through the websocket
  webSocket.broadcastTXT(jsonString); 
  jsonString = ""; // clear the String.
}

void sendUpdate()
{
  StaticJsonDocument<2048> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["msg"] = "update";

  for (byte i=0; i<channelCount; i++)
  {
    object["loads"][i]["id"] = i;
    object["loads"][i]["state"] = channelState[i];
    object["loads"][i]["duty"] = channelDutyCycle[i];
    object["loads"][i]["current"] = channelAmperage[i];
  }

  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString); 
  
  //Serial.println( jsonString );

  // send the JSON object through the websocket
  webSocket.broadcastTXT(jsonString); 
  jsonString = ""; // clear the String.
}

void connectToWifi()
{
  ssid = preferences.getString("ssid", "Wind.Ninja"); 
  password = preferences.getString("password", "chickenloop");

  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

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
  if (channelState[channelId])
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

void readAmperages()
{
  //readInternalADC();
  updateChannelsMCP3208();
}

void readInternalADC()
{
  for (byte i=0; i<channelCount; i++)
  {
    //channelAmperage[i] = i;
    channelAmperage[i] = getAmperageInternal(analogPins[i]);
  }
}

float getAmperageInternal(byte sensorPin)
{
  float AcsValue=0.0, Samples=0.0, AvgAcs=0.0, AcsValueF=0.0;
  for (int x = 0; x < 5; x++)
  {
    //AcsValue = analogRead(sensorPin);     //Read current sensor values   
    AcsValue = (float)analogReadMilliVolts(sensorPin) / 1000.0;

    Samples = Samples + AcsValue;  //Add samples together
    delay(5); // let ADC settle before next sample
  }
  AvgAcs=Samples / 5.0; //Taking Average of Samples

  //AvgAcs = analogRead(sensorPin);     //Read current sensor values   

  float amps;

  //Serial.print(sensorPin);
  //Serial.print(" PIN | ");

  int mv = analogReadMilliVolts(sensorPin);
  //Serial.print(mv);
  //Serial.print(" mV | ");

  //Serial.print(AvgAcs);
  //Serial.print(" ADC | ");

  float volts;
  //volts = AvgAcs * (3.3 / 4096.0);
  volts = AvgAcs * (5.0 / 3.3);

  //Serial.print(volts);
  //Serial.print(" V | ");

  //result = (readValue * 3.3) / 4096.0 / mVperAmp; //ESP32 ADC resolution 4096
  //1.65 = midpoint voltage
  //3.3 / 4096 = convert reading to volts
  //0.100 * 0.66 = ACS712 20A outputs 0.1v per amp, but we had to voltage divide to 3.3v
  //amps = (1.65 - volts) / (0.100 * 0.66);
  amps = (2.5 - volts) / 0.100;

  //Serial.print(amps);
  //Serial.print(" A");
  //Serial.println();

  return amps;
}

MCP3208 adc;
void setupMCP3208()
{
  adc.begin();
}

uint16_t readMCP3208Channel(byte channel, byte samples = 32)
{
  uint32_t value = 0;

  for (byte i=0; i<samples; i++)
  {
    value += adc.readADC(channel);
  }
  value = value / samples;

  return (uint16_t)value;
}

void updateChannelsMCP3208()
{
  for (byte channel = 0 ; channel < channelCount; channel++)
  {
    uint16_t val = readMCP3208Channel(channel);
    //Serial.print(val);
    //Serial.print(" ADC | ");

    float volts = val * (3.3 / 4096.0);
    volts = volts / 0.6875;
    //Serial.print(volts);
    //Serial.print(" V | ");

    float amps = 0.0;
    amps = (2.5 - volts) / (0.100); //ACS712 5V w/ voltage divider
    //amps = (volts - (3.3 * 0.1)) / (0.200); //TMCS1108A3U
    //amps = (volts - (3.3 * 0.1)) / (0.100); //TMCS1108A2U
    //amps = (volts - (3.3 * 0.1)) / (0.132); //ACS725LLCTR-20AU
    //amps = (volts - (3.3 * 0.5)) / (0.066);  //MCS1802-20
    //amps = (volts - 0.650) / (0.100);       //CT427-xSN820DR

    //Serial.print(amps);
    //Serial.print(" A | ");

    channelAmperage[channel] = amps;
  }
  //Serial.println();
}

