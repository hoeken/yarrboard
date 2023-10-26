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

#ifdef YB_HAS_DIGITAL_INPUT_CHANNELS
  #include "input_channel.h"
#endif

#ifdef YB_HAS_ADC
  #include "adc_channel.h"
#endif

#ifdef YB_HAS_RGB_OUTPUT
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

void generateUpdateJSON(JsonVariant output);
void generateConfigJSON(JsonVariant output);
void generateStatsJSON(JsonVariant output);
void generateNetworkConfigJSON(JsonVariant output);
void generateAppConfigJSON(JsonVariant output);
void generateOTAProgressUpdateJSON(JsonVariant output, float progress, int partition);
void generateOTAProgressFinishedJSON(JsonVariant output);
void generateErrorJSON(JsonVariant output, const char * error);
void generateSuccessJSON(JsonVariant output, const char * success);
void generateLoginRequiredJSON(JsonVariant output);
void generateInvalidChannelJSON(JsonVariant output, byte cid);
void generatePongJSON(JsonVariant output);

void sendUpdate();
void sendOTAProgressUpdate(float progress, int partition);
void sendOTAProgressFinished();
void sendToAll(const char * jsonString);

#endif /* !YARR_PROTOCOL_H */