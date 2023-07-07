/* Wi-Fi STA Connect and Disconnect Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
   
*/
#include <WiFi.h>
#include <ArduinoJson.h> // Include ArduinoJson Library
#include <WebSocketsServer.h>  // Include Websocket Library

const char* version = "Gnarboard v1.0.0";
String uuid;

const byte channelCount = 8;
const byte pwmPins[channelCount] = {32, 33, 25, 26, 27, 14, 13, 17};
//const byte analogPins[channelCount] = {};

bool channelState[channelCount] = {false, false, false, false, false, false, false, false};
byte channelPwm[channelCount] = {0, 0, 0, 0, 0, 0, 0, 0};
float channelAmperage[channelCount];

const char* ssid     = "Wind.Ninja";
const char* password = "chickenloop";

WebSocketsServer webSocket = WebSocketsServer(80);  //create instance for webSocket server on port"81"
String jsonString; // Temporary storage for the JSON String

int interval = 2000; // virtual delay
unsigned long previousMillis = 0; // Tracks the time since last event fired
bool toggle_state = false;

void setup()
{
  //startup our serial
  Serial.begin(115200);
  delay(10);
  Serial.println(version);

  //intitialize our output pins.
  for (byte i=0; i<channelCount; i++)
  {
    pinMode(pwmPins[i], OUTPUT);
    analogWrite(pwmPins[i], 0);

    channelAmperage[i] = 0.0;
  }

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

  readAmperages();

  //lookup our info periodically  
  unsigned long currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMillis) >= interval)
  {
    //read and send out our json update
    sendUpdate();

    //test our pin.
    if (toggle_state)
    {
      channelState[0] = true;
      analogWrite(23, 255);
    }
    else 
    {
      channelState[0] = false;
      analogWrite(23, 0);
    }
    toggle_state = !toggle_state;

    // Use the snapshot to set track time until next event
    previousMillis = currentMillis;   
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
      Serial.printf("[WebSocket] Message: %s\n", payload);
      handleReceivedMessage((char*)payload);
      break;
  }
}

void handleReceivedMessage(char *payload) {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  //JsonObject root = doc.as<JsonObject>();

  const char* cmd = doc["cmd"];
  int channelId = doc["id"];

  Serial.print("Command: ");
  Serial.println(cmd);

  Serial.print("Id: ");
  Serial.println(channelId);

  //Serial.print("Id: ");
  //Serial.println(id);

  //Serial.println(String(doc["cmd"]));
  //Serial.println(String(doc["channels"][0]["id"]));
}

void sendBoardInfo()
{
  StaticJsonDocument<2048> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["name"] = version;
  object["uuid"] = uuid;

  /*
  for (byte i=0; i<channelCount; i++)
  {
    object["loads"][i]["state"] = channelState[i];
  }
  */
  
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
  for (byte i=0; i<channelCount; i++)
  {
    object["loads"][i]["id"] = i;
    object["loads"][i]["state"] = channelState[i];
    object["loads"][i]["duty_cycle"] = (float)channelPwm[i] / 255.0;
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
  // We start by connecting to a WiFi network
  // To debug, please enable Core Debug Level to Verbose

  Serial.println();
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Auto reconnect is set true as default
  // To set auto connect off, use the following function
  //    WiFi.setAutoReconnect(false);

  // Will try for about 10 seconds (20x 500ms)
  int tryDelay = 500;
  int numberOfTries = 20;

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
      
      if(numberOfTries <= 0){
        Serial.print("[WiFi] Failed to connect to WiFi!");
        // Use disconnect function to force stop trying to connect
        WiFi.disconnect();
        return;
      } else {
        numberOfTries--;
      }
  }
}

void readAmperages()
{
  for (byte i=0; i<channelCount; i++)
  {
    channelAmperage[i] = i;
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

  Serial.print(sensorPin);
  Serial.print(" PIN | ");

  int mv = analogReadMilliVolts(sensorPin);
  Serial.print(mv);
  Serial.print(" mV | ");


  Serial.print(AvgAcs);
  Serial.print(" ADC | ");

  float volts;
  //volts = AvgAcs * (3.3 / 4096.0);
  volts = AvgAcs * (5.0 / 3.3);

  Serial.print(volts);
  Serial.print(" V | ");

  //result = (readValue * 3.3) / 4096.0 / mVperAmp; //ESP32 ADC resolution 4096
  //1.65 = midpoint voltage
  //3.3 / 4096 = convert reading to volts
  //0.100 * 0.66 = ACS712 20A outputs 0.1v per amp, but we had to voltage divide to 3.3v
  //amps = (1.65 - volts) / (0.100 * 0.66);
  amps = (2.5 - volts) / 0.100;

  Serial.print(amps);
  Serial.print(" A");
  Serial.println();

  return amps;
 }
