/*
  Yarrboard v1.0.0
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3

*/

#include "config.h"
#include "wifi.h"
#include "prefs.h"
#include "channel.h"
#include "websocket.h"
#include "utility.h"
#include "adc.h"
#include "fans.h"
#include "ota.h"
//#include "ntp.h"

void setup()
{
  unsigned long setup_t1 = micros();

  //startup our serial
  Serial.begin(115200);

  Serial.println("Yarrboard");
  Serial.print("Hardware Version: ");
  Serial.println(HARDWARE_VERSION);
  Serial.print("Firmware Version: ");
  Serial.println(FIRMWARE_VERSION);

  //ntp_setup();
  prefs_setup();
  channel_setup();
  adc_setup();
  fans_setup();
  wifi_setup();
  websocket_setup();
  ota_setup();

  unsigned long setup_t2 = micros();
  Serial.print("Boot time: ");
  Serial.print((float)(setup_t2 - setup_t1) / 1000.0);
  Serial.println("ms");
}

void loop()
{
  //ntp_loop();

  channel_loop();
  adc_loop();
  fans_loop();
  wifi_loop();
  websocket_loop();
  ota_loop();
}