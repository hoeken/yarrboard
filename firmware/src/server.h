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

#include "channel.h"
#include "protocol.h"
#include "prefs.h"
#include "wifi.h"
#include "ota.h"
#include "adc.h"
#include "fans.h"

extern String app_user;
extern String app_pass;
extern bool require_login;
extern bool app_enable_api;
extern bool app_enable_serial;

void server_setup();
void server_loop();

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);

bool isWebsocketClientLoggedIn(const JsonObject& doc, uint32_t client_id);
bool isApiClientLoggedIn(const JsonObject& doc);
bool isSerialClientLoggedIn(const JsonObject& doc);
bool logClientIn(uint32_t client_id);

void sendToAllWebsockets(char * jsonString);

#endif /* !YARR_SERVER_H */