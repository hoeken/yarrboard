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

#include "adchelper.h"
#include "ota.h"
#include "prefs.h"
#include "server.h"
#include "utility.h"
#include "wifi.h"
#include "debug.h"

#ifdef YB_HAS_INPUT_CHANNELS
  #include "input_channel.h"
#endif

#ifdef YB_HAS_ADC_CHANNELS
  #include "adc_channel.h"
#endif

#ifdef YB_HAS_RGB_CHANNELS
  #include "rgb_channel.h"
#endif

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  #include "bus_voltage.h"
#endif

//extern unsigned int handledMessages;
extern char board_name[YB_BOARD_NAME_LENGTH];
extern char app_user[YB_USERNAME_LENGTH];
extern char app_pass[YB_PASSWORD_LENGTH];
extern bool require_login;
extern bool app_enable_api;
extern bool app_enable_serial;
extern bool is_serial_authenticated;
extern uint32_t authenticatedClientIDs[YB_CLIENT_LIMIT];


void protocol_setup();
void protocol_loop();

bool isLoggedIn(JsonVariantConst input, byte mode, uint32_t client_id);

void handleSerialJson();

void handleReceivedJSON(JsonVariantConst input, JsonVariant output, byte mode, uint32_t client_id);
void handleSetBoardName(JsonVariantConst input, JsonVariant output);
void handleSetNetworkConfig(JsonVariantConst input, JsonVariant output);
void handleSetAppConfig(JsonVariantConst input, JsonVariant output);
void handleLogin(JsonVariantConst input, JsonVariant output, byte mode, uint32_t client_id);
void handleRestart(JsonVariantConst input, JsonVariant output);
void handleFactoryReset(JsonVariantConst input, JsonVariant output);
void handleOTAStart(JsonVariantConst input, JsonVariant output);
void handleSetPWMChannel(JsonVariantConst input, JsonVariant output);
void handleTogglePWMChannel(JsonVariantConst input, JsonVariant output);
void handleFadePWMChannel(JsonVariantConst input, JsonVariant output);
void handleSetSwitch(JsonVariantConst input, JsonVariant output);
void handleSetRGB(JsonVariantConst input, JsonVariant output);
void handleSetADC(JsonVariantConst input, JsonVariant output);

void generateUpdateJSON(JsonVariant output);
void generateConfigJSON(JsonVariant output);
void generateStatsJSON(JsonVariant output);
void generateNetworkConfigJSON(JsonVariant output);
void generateAppConfigJSON(JsonVariant output);
void generateOTAProgressUpdateJSON(JsonVariant output, float progress);
void generateOTAProgressFinishedJSON(JsonVariant output);
void generateErrorJSON(JsonVariant output, const char * error);
void generateSuccessJSON(JsonVariant output, const char * success);
void generateLoginRequiredJSON(JsonVariant output);
void generateInvalidChannelJSON(JsonVariant output, byte cid);
void generatePongJSON(JsonVariant output);

void sendUpdate();
void sendOTAProgressUpdate(float progress);
void sendOTAProgressFinished();
void sendToAll(const char * jsonString);

bool isWebsocketClientLoggedIn(JsonVariantConst input, uint32_t client_id);
bool isApiClientLoggedIn(JsonVariantConst input);
bool isSerialClientLoggedIn(JsonVariantConst input);
bool addClientToAuthList(uint32_t client_id);

#endif /* !YARR_PROTOCOL_H */