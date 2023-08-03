#ifndef YARR_WEBSOCKET_H
#define YARR_WEBSOCKET_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "channel.h"

extern String app_user;
extern String app_pass;
extern bool require_login;

extern AsyncWebSocket ws;

void websocket_setup();
void websocket_loop();

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
void handleReceivedMessage(char *payload, AsyncWebSocketClient *client);

bool assertLoggedIn(AsyncWebSocketClient *client);
bool assertValidChannel(byte cid, AsyncWebSocketClient *client);

void sendToAll(String jsonString);
void sendUpdate();
void sendConfigJSON(AsyncWebSocketClient *client);
void sendNetworkConfigJSON(AsyncWebSocketClient *client);
void sendStatsJSON(AsyncWebSocketClient *client);
void sendSuccessJSON(String success, AsyncWebSocketClient *client);
void sendErrorJSON(String error, AsyncWebSocketClient *client);

#endif /* !YARR_WEBSOCKET_H */