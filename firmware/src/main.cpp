/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#include "wifi.h"
#include "prefs.h"
#include "channel.h"
#include "server.h"
#include "utility.h"
#include "adc.h"
#include "fans.h"
#include "ota.h"
//#include "ntp.h"

void setup()
{
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
  protocol_setup();
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
  protocol_loop();
}