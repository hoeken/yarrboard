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
#include <AsyncJson.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <CircularBuffer.h>

#include "channel.h"
#include "protocol.h"
#include "prefs.h"
#include "wifi.h"
#include "ota.h"
#include "adc.h"
#include "fans.h"

typedef struct {
  unsigned int client_id;
  char buffer[YB_RECEIVE_BUFFER_LENGTH];
} WebsocketRequest;

extern char app_user[YB_USERNAME_LENGTH];
extern char app_pass[YB_PASSWORD_LENGTH];

extern bool require_login;
extern bool app_enable_api;
extern bool app_enable_serial;
extern bool is_serial_authenticated;

void server_setup();
void server_loop();

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
void handleWebsocketMessageLoop(WebsocketRequest* request);
void handleWebServerRequest(JsonVariant input, AsyncWebServerRequest *request);

bool isWebsocketClientLoggedIn(JsonVariantConst input, uint32_t client_id);
bool isApiClientLoggedIn(JsonVariantConst input);
bool isSerialClientLoggedIn(JsonVariantConst input);

bool logClientIn(uint32_t client_id);
int getWebsocketRequestSlot();
void closeClientConnection(AsyncWebSocketClient *client);

void sendToAllWebsockets(const char * jsonString);

int getFreeSlots();


#endif /* !YARR_SERVER_H */