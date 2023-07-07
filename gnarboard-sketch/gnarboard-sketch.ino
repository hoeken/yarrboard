/* Wi-Fi STA Connect and Disconnect Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
   
*/
#include <WiFi.h>
#include <ArduinoJson.h> // Include ArduinoJson Library
#include <WebSocketsServer.h>  // Include Websocket Library

const char* ssid     = "Wind.Ninja";
const char* password = "chickenloop";

WebSocketsServer webSocket = WebSocketsServer(80);  //create instance for webSocket server on port"81"
String jsonString; // Temporary storage for the JSON String

int interval = 1000; // virtual delay
unsigned long previousMillis = 0; // Tracks the time since last event fired

void setup()
{
    Serial.begin(115200);
    delay(10);

  connectToWifi();

  webSocket.begin();  // init the Websocketserver
  webSocket.onEvent(webSocketEvent);  // init the webSocketEvent function when a websocket event occurs 
}

void loop()
{
  webSocket.loop(); // websocket server methode that handles all Client
  
  unsigned long currentMillis = millis(); // call millis  and Get snapshot of time

  //lookup our info periodically  
  if ((unsigned long)(currentMillis - previousMillis) >= interval) { // How much time has passed, accounting for rollover with subtraction!
    previousMillis = currentMillis;   // Use the snapshot to set track time until next event

    sendUpdate();
  }
}

// This function gets a call when a WebSocket event occurs
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: // enum that read status this is used for debugging.
      Serial.print("WS Type ");
      Serial.print(type);
      Serial.println(": DISCONNECTED");
      break;
    case WStype_CONNECTED:  // Check if a WebSocket client is connected or not
      Serial.print("WS Type ");
      Serial.print(type);
      Serial.println(": CONNECTED");
      break;
    case WStype_TEXT: // check responce from client
      Serial.println(); // the payload variable stores teh status internally
      //Serial.println(payload);
      break;
  }
}

void sendUpdate()
{
  StaticJsonDocument<2048> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();
  for (int i=0; i<7; i++)
  {
    object["state"][i] = false;
    object["current"][i] = 0.43;
  }

  serializeJson(doc, jsonString); // serialize the object and save teh result to teh string variable.
  Serial.println( jsonString ); // print the string for debugging.
  webSocket.broadcastTXT(jsonString); // send the JSON object through the websocket
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
