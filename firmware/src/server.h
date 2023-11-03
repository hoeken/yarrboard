/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_SERVER_H
#define YARR_SERVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <CircularBuffer.h>
#include <MongooseCore.h>
#include <MongooseHttpServer.h>

#include "protocol.h"
#include "prefs.h"
#include "wifi.h"
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

typedef struct {
  MongooseHttpWebSocketConnection *client;
  char buffer[YB_RECEIVE_BUFFER_LENGTH];
} WebsocketRequest;

void server_setup();
void server_loop();

bool logClientIn(uint32_t client_id);
int getFreeSlots();
int getWebsocketRequestSlot();
void sendToAllWebsockets(const char * jsonString);
void handleWebsocketMessageLoop(WebsocketRequest* request);

void handleWebServerRequest(JsonVariant input, MongooseHttpServerRequest *request);
void handleWebSocketMessage(MongooseHttpWebSocketConnection *connection, uint8_t *data, size_t len);

/*
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void closeClientConnection(AsyncWebSocketClient *client);
*/


#endif /* !YARR_SERVER_H */