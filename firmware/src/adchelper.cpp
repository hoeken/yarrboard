/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adchelper.h"

//for tracking our ADC loop
unsigned long previousADCMillis = 0;

//object for our adc
MCP3208 _adcMCP3208;
const byte adc_cs_pin = 17;

#ifdef YB_HAS_BUS_VOLTAGE
  //for watching our power supply
  float busVoltage = 0;

  #ifdef YB_BUS_VOLTAGE_ESP32
    esp32Helper busADC = esp32Helper(3.3, YB_BUS_VOLTAGE_PIN);
  #endif

  #ifdef YB_BUS_VOLTAGE_MCP3221
  #endif
#endif

void adc_setup()
{
  _adcMCP3208.begin(adc_cs_pin);

  #ifdef YB_HAS_BUS_VOLTAGE
    #ifdef YB_BUS_VOLTAGE_ESP32
      adcAttachPin(YB_BUS_VOLTAGE_PIN);
      analogSetAttenuation(ADC_11db);
    #endif

    #ifdef YB_BUS_VOLTAGE_MCP3221
    #endif
  #endif
}

void adc_loop()
{
  #ifdef YB_HAS_BUS_VOLTAGE
    //check what our power is.
    busADC.getReading();
  #endif
}


#ifdef YB_HAS_BUS_VOLTAGE
  float getBusVoltage()
  {
    unsigned int reading = busADC.getAverageReading();
    float voltage = busADC.toVoltage(reading);

    busADC.resetAverage();

    return voltage / (YB_BUS_VOLTAGE_R2 / (YB_BUS_VOLTAGE_R2+YB_BUS_VOLTAGE_R1));
  }
#endif

ADCHelper::ADCHelper()
{
  this->vref = 3.3;
  this->resolution = 12;
}

ADCHelper::ADCHelper(float vref, uint8_t resolution)
{
  this->vref = vref;
  this->resolution = resolution;
}

unsigned int ADCHelper::getReading()
{
  return 0;
}

void ADCHelper::addReading(unsigned int reading)
{
  this->cumulativeReadings += reading;
  this->readingCount++;
}

float ADCHelper::getVoltage()
{
  return this->toVoltage(this->getReading());
}

unsigned int ADCHelper::getAverageReading()
{
  if (this->readingCount > 0)
    return round((float)this->cumulativeReadings / (float)this->readingCount);
  else
    return 0;
}

float ADCHelper::getAverageVoltage()
{
  return this->toVoltage(this->getAverageReading());
}

void ADCHelper::resetAverage()
{
  this->readingCount = 0;
  this->cumulativeReadings = 0;
}

float ADCHelper::toVoltage(unsigned int reading)
{
  return (float)reading * this->vref / (float)(pow(2, this->resolution) - 1);
}

esp32Helper::esp32Helper() : ADCHelper::ADCHelper() {}
esp32Helper::esp32Helper(float vref, uint8_t channel) : ADCHelper::ADCHelper(vref, 12)
{
  this->channel = channel;
}

unsigned int esp32Helper::getReading()
{
  unsigned int reading = analogReadMilliVolts(this->channel);
  this->addReading(reading);

  return reading;
}

float esp32Helper::toVoltage(unsigned int reading)
{
  return (float)reading / 1000.0;
}

MCP3208Helper::MCP3208Helper() : ADCHelper::ADCHelper() {}
MCP3208Helper::MCP3208Helper(float vref, uint8_t channel, MCP3208 *adc) : ADCHelper::ADCHelper(vref, 12)
{
  this->channel = channel;
  this->adc = adc;
}

unsigned int MCP3208Helper::getReading()
{
  unsigned int reading = this->adc->readADC(this->channel);
  this->addReading(reading);

  return reading;
}