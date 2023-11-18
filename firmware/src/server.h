/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_SERVER_H
#define YARR_SERVER_H

#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <CircularBuffer.h>
#include <MongooseCore.h>
#include <MongooseHttpServer.h>

#include "protocol.h"
#include "prefs.h"
#include "network.h"
#include "ota.h"
#include "adchelper.h"

//generated at build by running "gulp" in the firmware directory.
#include "index.html.gz.h"

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

extern String server_cert;
extern String server_key;

typedef struct {
  MongooseHttpWebSocketConnection *client;
  char buffer[YB_RECEIVE_BUFFER_LENGTH];
} WebsocketRequest;

void server_setup();
void server_loop();

bool isLoggedIn(JsonVariantConst input, byte mode, MongooseHttpWebSocketConnection *connection);
bool logClientIn(MongooseHttpWebSocketConnection *connection);
void closeClientConnection(MongooseHttpWebSocketConnection *connection);
bool addClientToAuthList(MongooseHttpWebSocketConnection *connection);
bool isWebsocketClientLoggedIn(JsonVariantConst input, MongooseHttpWebSocketConnection *connection);
bool isApiClientLoggedIn(JsonVariantConst input);
bool isSerialClientLoggedIn(JsonVariantConst input);


int getFreeSlots();
int getWebsocketRequestSlot();

void sendToAllWebsockets(const char * jsonString);
void handleWebsocketMessageLoop(WebsocketRequest* request);

void handleWebServerRequest(JsonVariant input, MongooseHttpServerRequest *request);
void handleWebSocketMessage(MongooseHttpWebSocketConnection *connection, uint8_t *data, size_t len);

/*
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
*/


#endif /* !YARR_SERVER_H */