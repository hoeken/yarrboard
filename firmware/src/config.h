/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_CONFIG_H
#define YARR_CONFIG_H

#include <ArduinoTrace.h>

#define YB_FIRMWARE_VERSION "1.2.0"

//#define CORE_DEBUG_LEVEL 5

#if defined YB_CONFIG_8CH_MOSFET_REV_A
  #include "./configs/config.8ch-mosfet-reva.h"
#elif defined YB_CONFIG_8CH_MOSFET_REV_B
  #include "./configs/config.8ch-mosfet-revb.h"
#elif defined YB_CONFIG_8CH_MOSFET_REV_C
  #include "./configs/config.8ch-mosfet-revC.h"
#elif defined YB_CONFIG_RGB_INPUT_REV_A
  #include "./configs/config.rgb-input-reva.h"
#elif defined YB_CONFIG_RGB_INPUT_REV_B
  #include "./configs/config.rgb-input-revb.h"
#else
  #error "No board config has been defined"
#endif

//time before saving fade pwm to preserve flash
#define YB_DUTY_SAVE_TIMEOUT 5000

//bytes for sending json
#define YB_LARGE_JSON_SIZE 4096
#define YB_MAX_JSON_LENGTH 2500
#define YB_CLIENT_LIMIT 8

#define YBP_MODE_WEBSOCKET 0
#define YBP_MODE_HTTP      1
#define YBP_MODE_SERIAL    2

//for handling messages outside of the loop
#define YB_RECEIVE_BUFFER_LENGTH 512
#define YB_RECEIVE_BUFFER_COUNT 16

//milliseconds between sending updates on websocket and serial
#define YB_UPDATE_FREQUENCY 250
#define YB_ADC_INTERVAL 50
#define YB_ADC_SAMPLES 1

#define YB_PREF_KEY_LENGTH 16
#define YB_BOARD_NAME_LENGTH 31
#define YB_USERNAME_LENGTH 31
#define YB_PASSWORD_LENGTH 31
#define YB_CHANNEL_NAME_LENGTH 31
#define YB_WIFI_SSID_LENGTH 33
#define YB_WIFI_PASSWORD_LENGTH 64
#define YB_HOSTNAME_LENGTH 64
#define YB_VALIDATE_FIRMWARE_SIGNATURE true

#endif // YARR_CONFIG_H