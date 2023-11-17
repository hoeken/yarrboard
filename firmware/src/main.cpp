/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#include "ota.h"
#include "network.h"
#include "prefs.h"
#include "server.h"
#include "utility.h"
#include "adchelper.h"
#include "debug.h"
#include <LittleFS.h>

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

unsigned long lastFrameMillis = 0;

void setup()
{
  //startup our serial
  Serial.begin(115200);
  Serial.setTimeout(50);

  /* need to zero out the ticklist array before starting */
  for (int i=0; i<YB_FPS_SAMPLES; i++)
    ticklist[i] = 0;

  if(!LittleFS.begin(true)) {
    Serial.println("ERROR: Unable to mount LittleFS");
  }

  Serial.println("Yarrboard");
  Serial.print("Hardware Version: ");
  Serial.println(YB_HARDWARE_VERSION);
  Serial.print("Firmware Version: ");
  Serial.println(YB_FIRMWARE_VERSION);

  debug_setup();
  Serial.println("Debug ok");

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

  network_setup();
  Serial.println("Network ok");
  
  server_setup();
  Serial.println("Server ok");
  
  protocol_setup();
  Serial.println("Protocol ok");
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

  network_loop();
  server_loop();
  protocol_loop();
  ota_loop();

  //calculate our framerate
  framerate = calculateFramerate(millis() - lastFrameMillis);
  lastFrameMillis = millis();
}