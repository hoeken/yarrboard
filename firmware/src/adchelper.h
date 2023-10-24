/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_ADC_H
#define YARR_ADC_H

#include <Arduino.h>
#include <MCP3208.h>
#include <Wire.h>
#include "config.h"
//#include <MCP342x.h>

class ADCHelper
{
  public:
    ADCHelper();
    ADCHelper(float vref, uint8_t resolution);
    
    unsigned int getReading();
    unsigned int getAverageReading();
    void addReading(unsigned int reading);
    float getVoltage();
    float getAverageVoltage();
    void resetAverage();
    float toVoltage(unsigned int reading);

    unsigned int readingCount = 0;

  private:
    float vref = 0.0;
    uint8_t resolution;
    unsigned long cumulativeReadings = 0;
};

class esp32Helper : public ADCHelper
{
  public:
    esp32Helper();
    esp32Helper(float vref, uint8_t channel);
    unsigned int getReading();
    float toVoltage(unsigned int reading);

  private:
    uint8_t channel;
};

class MCP3208Helper : public ADCHelper
{
  public:
    MCP3208Helper();
    MCP3208Helper(float vref, uint8_t channel, MCP3208 *adc);
    unsigned int getReading();

  private:
    MCP3208 *adc;
    uint8_t channel;
};

extern MCP3208 _adcMCP3208;

void adc_setup();
void adc_loop();

#ifdef YB_HAS_BUS_VOLTAGE
  extern float busVoltage;
  float getBusVoltage();
#endif

#endif /* !YARR_ADC_H */