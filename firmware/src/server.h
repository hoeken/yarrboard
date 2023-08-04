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
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "channel.h"
#include "protocol.h"

extern String board_name;
extern String app_user;
extern String app_pass;
extern bool require_login;
extern AsyncWebSocket ws;

void websocket_setup();
void websocket_loop();

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);

bool isClientLoggedIn(uint32_t client_id);
bool logClientIn(uint32_t client_id);

void sendToAll(char * jsonString);
void sendUpdate();

#endif /* !YARR_SERVER_H */