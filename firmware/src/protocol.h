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
extern char board_name[YB_BOARD_NAME_LENGTH];

void protocol_setup();
void protocol_loop();

bool isLoggedIn(const JsonObject& doc, byte mode, uint32_t client_id);
bool isValidChannel(byte cid);

void handleSerialJson();

void handleReceivedJSON(const JsonObject& doc, char *output, byte mode, uint32_t client_id);
void handleSetBoardName(const JsonObject& doc, char * output);
void handleSetChannel(const JsonObject& doc, char * output);
void handleToggleChannel(const JsonObject& doc, char * output);
void handleSetNetworkConfig(const JsonObject& doc, char * output);
void handleSetAppConfig(const JsonObject& doc, char * output);
void handleLogin(const JsonObject& doc, char * output, byte mode, uint32_t client_id);
void handleRestart(const JsonObject& doc, char * output);
void handleFactoryReset(const JsonObject& doc, char * output);
void handleOTAStart(const JsonObject& doc, char * output);

void generateUpdateJSON(char * jsonBuffer);
void generateConfigJSON(char * jsonBuffer);
void generateStatsJSON(char * jsonBuffer);
void generateNetworkConfigJSON(char * jsonBuffer);
void generateAppConfigJSON(char * jsonBuffer);
void generateOTAProgressUpdateJSON(char * jsonBuffer, float progress, int partition);
void generateOTAProgressFinishedJSON(char * jsonBuffer);
void generateErrorJSON(char * jsonBuffer, const char * error);
void generateSuccessJSON(char * jsonBuffer, const char * success);
void generateLoginRequiredJSON(char * jsonBuffer);
void generateInvalidChannelJSON(char * jsonBuffer, byte cid);
void generatePongJSON(char * jsonBuffer);
void generateOKJSON(char * jsonBuffer);

void sendUpdate();
void sendOTAProgressUpdate(float progress, int partition);
void sendOTAProgressFinished();
void sendToAll(const char * jsonString);

#endif /* !YARR_PROTOCOL_H */