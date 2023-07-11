/*
  Gnarboard v1
*/
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <MCP_ADC.h>
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer/
#include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP/
#include "SPIFFS.h"

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
int    channelPWM[channelCount] = {0, 0, 0, 0, 0, 0, 0, 0};
float  channelAmperage[channelCount];
float  channelSoftFuseAmperage[channelCount];
String channelNames[channelCount];

/* Setting PWM Properties */
const int PWMFreq = 10000; /* in Hz  */
const int PWMResolution = 12;
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
    ledcSetup(i, PWMFreq, PWMResolution);
    ledcAttachPin(outputPins[i], i);
    ledcWrite(i, 0);

    //just init some values, will get real ones later.
    channelPWM[i] = 0;
    channelAmperage[i] = 0.0;
    //channelSoftFuseAmperage[i] = 20;

    //lookup our name
    String prefIndex = "channel_name_" + i;
    channelNames[i] = preferences.getString(prefIndex.c_str());
  }

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
      updateChannels();

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

    bool state = doc["value"];
    channelState[cid] = state;
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

void updateChannels()
{
  for (byte i=0; i<channelCount; i++)
  {
    if (channelState[i])
    {
      int pwm = (int)(channelDutyCycle[i] * MAX_DUTY_CYCLE);
      if (pwm != channelPWM[i])
      {
        ledcWrite(i, pwm);
        //analogWrite(outputPins[i], pwm);
        channelPWM[i] = pwm;
        //Serial.println(pwm);
      }
    }
    else
    {
      analogWrite(outputPins[i], 0);
    }
  }
}

void readAmperages()
{
  for (byte i=0; i<channelCount; i++)
  {
    //channelAmperage[i] = i;
    channelAmperage[i] = getAmperage(analogPins[i]);
  }
}

float getAmperage(byte sensorPin)
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

MCP3008 mcp1; 
void setupMCP3208()
{
  mcp1.begin(5);
  mcp1.setSPIspeed(4000000);

}

void updateChannelsMCP3208()
{
  for (int channel = 0 ; channel < channelCount; channel++)
  {
    uint16_t val = mcp1.analogRead(channel);
    Serial.print(val);
    Serial.print(" ADC | ");

    float volts = val * (3.3 / 4096.0);
    Serial.print(volts);
    Serial.print(" V | ");

    float amps = 0.0;
    //amps = (1.65 - volts) / (0.100 * 0.66); //ACS712 5V
    //amps = (volts - (3.3 * 0.1)) / (0.200); //TMCS1108A3U
    //amps = (volts - (3.3 * 0.1)) / (0.100); //TMCS1108A2U
    //amps = (volts - (3.3 * 0.1)) / (0.132); //ACS725LLCTR-20AU
    //amps = (volts - (3.3 * 0.5)) / (0.066);  //MCS1802-20
    //amps = (volts - 0.650) / (0.100);       //CT427-xSN820DR

    Serial.print(amps);
    Serial.print(" A");

    channelAmperage[channel] = amps;
  }
}

float TMCS1108A3U_v2a(float volts)
{
  
}