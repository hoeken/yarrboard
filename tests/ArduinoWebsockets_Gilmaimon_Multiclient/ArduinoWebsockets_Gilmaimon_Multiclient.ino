/*
  Minimal Esp32 Websockets Server

  This sketch:
        1. Connects to a WiFi network
        2. Starts a websocket server on port 80
        3. Waits for connections
        4. Once a client connects, it wait for a message from the client
        5. Sends an "echo" message to the client
        6. closes the connection and goes back to step 3

  Hardware:
        For this sketch you only need an ESP32 board.

  Created 15/02/2019
  By Gil Maimon
  https://github.com/gilmaimon/ArduinoWebsockets
*/

#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <vector>

//wifi pass
const char* ssid = "Wind.Ninja"; //Enter SSID
const char* password = "chickenloop"; //Enter Password

//websocket vars
using namespace websockets;
WebsocketsServer server;
std::vector<WebsocketsClient> allClients;

// this method goes thrugh every client and polls for new messages and events
void pollAllClients() {
  for(auto& client : allClients) {
    client.poll();
  }
}

/*
void sendReponse(WebsocketsClient& client)
{

}
*/

// this callback is common for all clients, the client that sent that
// message is the one that gets the echo response
void onMessage(WebsocketsClient& client, WebsocketsMessage message) {
  //Serial.print("Got Message: ");
  //Serial.println(message.data());
  handleMessage(message.data());

  StaticJsonDocument<1000> doc;
  String jsonString; // Temporary storage for the JSON String

  // create an object
  JsonObject object = doc.to<JsonObject>();

  object["millis"] = millis();

  // serialize the object and save teh result to teh string variable.
  serializeJson(doc, jsonString); 

  //Serial.println( jsonString );
  client.send(jsonString); 

  jsonString = ""; // clear the String.
}

void setup() {
  Serial.begin(115200);
  // Connect to wifi
  WiFi.begin(ssid, password);

  // Wait some time to connect to wifi
  for(int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++) {
      Serial.print(".");
      delay(1000);
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());   //You can get IP address assigned to ESP

  server.listen(8080);
  Serial.print("Is server live? ");
  Serial.println(server.available());
}

int messages;
int lastMillis = 0;

void loop()
{
  //look for new clients
  if(server.available())
  {
    if(server.poll())
    {
      // store it for later use
      WebsocketsClient client = server.accept();
      client.onMessage(onMessage);
      allClients.push_back(client);
    }
  }

  // check for updates in all clients
  pollAllClients();


  if (millis() - lastMillis > 1000)
  {
    Serial.print("msgs: ");
    Serial.println(messages);

    messages = 0;
    lastMillis = millis();
  }
}

void handleMessage(String payload)
{
  StaticJsonDocument<2048> doc;
  deserializeJson(doc, payload);

  //what is your command?
  String cmd = doc["cmd"];

  messages++;
}

