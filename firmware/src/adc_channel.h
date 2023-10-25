/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_ADC_CHANNEL_H
#define YARR_ADC_CHANNEL_H

#include <Arduino.h>
#include "config.h"
#include "prefs.h"
#include "adchelper.h"

class ADCChannel
{
  public:
    byte id = 0;
    bool isEnabled = true;
    char name[YB_CHANNEL_NAME_LENGTH];

    MCP3208Helper *adcHelper;

    void setup();
    void update();
    unsigned int getReading();
    float getVoltage();
    void resetAverage();
};

extern ADCChannel adc_channels[YB_ADC_CHANNEL_COUNT];

void adc_channels_setup();
void adc_channels_loop();
bool isValidADCChannel(byte cid);

#endif /* !YARR_INPUT_CHANNEL_H */