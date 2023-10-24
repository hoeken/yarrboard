/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#include "wifi.h"
#include "prefs.h"
#include "ota.h"
#include "server.h"
#include "utility.h"
//#include "ntp.h"

#ifdef YB_HAS_OUTPUT_CHANNELS
  #include "channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

#include "adchelper.h"

void setup()
{
  //startup our serial
  Serial.begin(115200);
  Serial.setTimeout(50);

  Serial.println("Yarrboard");
  Serial.print("Hardware Version: ");
  Serial.println(HARDWARE_VERSION);
  Serial.print("Firmware Version: ");
  Serial.println(FIRMWARE_VERSION);

  //ntp_setup();
  prefs_setup();
  Serial.println("Prefs ok");

  ota_setup();
  Serial.println("OTA ok");

  adc_setup();
  Serial.println("ADC ok");
  
  #ifdef YB_HAS_OUTPUT_CHANNELS
    channel_setup();
    Serial.println("Channels ok");
  #endif

  #ifdef YB_HAS_FANS
    fans_setup();
    Serial.println("Fans ok");
  #endif

  wifi_setup();
  Serial.println("WiFi ok");
  
  server_setup();
  Serial.println("Server ok");
  
  protocol_setup();
  Serial.println("Protocol ok");
}

void loop()
{
  //ntp_loop();
  #ifdef YB_HAS_OUTPUT_CHANNELS
    channel_loop();
  #endif

  adc_loop();

  #ifdef YB_HAS_FANS
   fans_loop();
  #endif

  wifi_loop();
  server_loop();
  ota_loop();
  protocol_loop();
}