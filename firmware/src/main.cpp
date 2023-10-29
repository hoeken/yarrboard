/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#include "ota.h"
#include "wifi.h"
#include "prefs.h"
#include "server.h"
#include "utility.h"
#include "adchelper.h"

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_INPUT_CHANNELS
  #include "input_channel.h"
#endif

#ifdef YB_HAS_ADC_CHANNELS
  #include "adc_channel.h"
#endif

#ifdef YB_HAS_RGB_CHANNELS
  #include "rgb_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  #include "bus_voltage.h"
#endif

#include "RTClib.h"
// RTC_PCF8523 rtc;
// unsigned long lastRTCCheckMillis = 0;

void setup()
{
  //startup our serial
  Serial.begin(115200);
  Serial.setTimeout(50);

  Serial.println("Yarrboard");
  Serial.print("Hardware Version: ");
  Serial.println(YB_HARDWARE_VERSION);
  Serial.print("Firmware Version: ");
  Serial.println(YB_FIRMWARE_VERSION);

  //ntp_setup();
  prefs_setup();
  Serial.println("Prefs ok");

  ota_setup();
  Serial.println("OTA ok");

  #ifdef YB_HAS_INPUT_CHANNELS
    input_channels_setup();
    Serial.println("Input channels ok");
  #endif

  #ifdef YB_HAS_ADC_CHANNELS
    adc_channels_setup();
    Serial.println("ADC channels ok");
  #endif

  #ifdef YB_HAS_RGB_CHANNELS
    rgb_channels_setup();
    Serial.println("RGB channels ok");
  #endif

  #ifdef YB_HAS_PWM_CHANNELS
    pwm_channels_setup();
    Serial.println("Output channels ok");
  #endif

  #ifdef YB_HAS_FANS
    fans_setup();
    Serial.println("Fans ok");
  #endif

  #ifdef YB_HAS_BUS_VOLTAGE
    bus_voltage_setup();
    Serial.println("Bus voltage ok");
  #endif

  wifi_setup();
  Serial.println("WiFi ok");
  
  server_setup();
  Serial.println("Server ok");
  
  protocol_setup();
  Serial.println("Protocol ok");

  // if (!rtc.begin()) {
  //   Serial.println("Couldn't find RTC");
  // } else {
  //   if (! rtc.initialized() || rtc.lostPower()) {
  //     Serial.println("RTC is NOT initialized, let's set the time!");
  //     // When time needs to be set on a new device, or after a power loss, the
  //     // following line sets the RTC to the date & time this sketch was compiled
  //     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //     // This line sets the RTC with an explicit date & time, for example to set
  //     // January 21, 2014 at 3am you would call:
  //     // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  //     //
  //     // Note: allow 2 seconds after inserting battery or applying external power
  //     // without battery before calling adjust(). This gives the PCF8523's
  //     // crystal oscillator time to stabilize. If you call adjust() very quickly
  //     // after the RTC is powered, lostPower() may still return true.
  //   }

  //   // When time needs to be re-set on a previously configured device, the
  //   // following line sets the RTC to the date & time this sketch was compiled
  //   // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //   // This line sets the RTC with an explicit date & time, for example to set
  //   // January 21, 2014 at 3am you would call:
  //   // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  //   // When the RTC was stopped and stays connected to the battery, it has
  //   // to be restarted by clearing the STOP bit. Let's do this to ensure
  //   // the RTC is running.
  //   rtc.start();
  // }
}

void loop()
{
  #ifdef YB_HAS_INPUT_CHANNELS
    input_channels_loop();
  #endif

  #ifdef YB_HAS_ADC_CHANNELS
    adc_channels_loop();
  #endif

  #ifdef YB_HAS_RGB_CHANNELS
    rgb_channels_loop();
  #endif

  #ifdef YB_HAS_PWM_CHANNELS
    pwm_channels_loop();
  #endif

  #ifdef YB_HAS_FANS
   fans_loop();
  #endif

  #ifdef YB_HAS_BUS_VOLTAGE
    bus_voltage_loop();
  #endif

  wifi_loop();
  server_loop();
  protocol_loop();
  ota_loop();

  // if (millis() > lastRTCCheckMillis + 1000)
  // {
  //   DateTime now = rtc.now();
  //   Serial.print(now.year(), DEC);
  //   Serial.print('/');
  //   Serial.print(now.month(), DEC);
  //   Serial.print('/');
  //   Serial.print(now.day(), DEC);
  //   Serial.print(" ");
  //   Serial.print(now.hour(), DEC);
  //   Serial.print(':');
  //   Serial.print(now.minute(), DEC);
  //   Serial.print(':');
  //   Serial.print(now.second(), DEC);
  //   Serial.println();

  //   lastRTCCheckMillis = millis();
  // }
}