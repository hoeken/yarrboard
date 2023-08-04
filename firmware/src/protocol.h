/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PROTOCOL_H
#define YARR_PROTOCOL_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "adc.h"
#include "channel.h"
#include "fans.h"
//#include "ntp.h"
#include "ota.h"
#include "prefs.h"
#include "server.h"
#include "utility.h"
#include "wifi.h"

//extern unsigned int handledMessages;
extern String board_name;

void protocol_setup();
void protocol_loop();

bool isLoggedIn(byte mode, uint32_t client_id);
bool isValidChannel(byte cid);

void handleReceivedJSON(const JsonObject& doc, char *output, byte mode, uint32_t client_id);
void handleSetBoardName(const JsonObject& doc, char * output);
void handleSetState(const JsonObject& doc, char * output);
void handleSetDuty(const JsonObject& doc, char * output);
void handleSetChannelName(const JsonObject& doc, char * output);
void handleSetDimmable(const JsonObject& doc, char * output);
void handleSetEnabled(const JsonObject& doc, char * output);
void handleSetSoftFuse(const JsonObject& doc, char * output);
void handleSetNetworkConfig(const JsonObject& doc, char * output);
void handleLogin(const JsonObject& doc, char * output, byte mode, uint32_t client_id);

void generateUpdateJSON(char * jsonBuffer);
void generateStatsJSON(char * jsonBuffer);
void generateNetworkConfigJSON(char * jsonBuffer);
void generateConfigJSON(char * jsonBuffer);
void generateOTAProgressUpdateJSON(char * jsonBuffer, float progress, int partition);
void generateOTAProgressFinishedJSON(char * jsonBuffer);
void generateErrorJSON(char * jsonBuffer, String error);
void generateSuccessJSON(char * jsonBuffer, String success);
void generateLoginRequiredJSON(char * jsonBuffer);
void generateInvalidChannelJSON(char * jsonBuffer, byte cid);
void generatePongJSON(char * jsonBuffer);
void generateOKJSON(char * jsonBuffer);

void sendUpdate();
void sendOTAProgressUpdate(float progress, int partition);
void sendOTAProgressFinished();
void sendToAll(char * jsonString);

#endif /* !YARR_PROTOCOL_H */