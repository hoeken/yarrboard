/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adc.h"

//object for our adc
MCP3208 adc;
const byte adc_cs_pin = 17;

//for watching our power supply
float busVoltage = 0;

//for tracking our ADC loop
unsigned long previousADCMillis = 0;

#ifdef BUS_VOLTAGE_MCP3221
#endif

void adc_setup()
{
  adc.begin(adc_cs_pin);

  #ifdef BUS_VOLTAGE_ESP32
    adcAttachPin(BUS_VOLTAGE_PIN);
    analogSetAttenuation(ADC_11db);
  #endif

  #ifdef BUS_VOLTAGE_MCP3221
  #endif
}

void adc_loop()
{
  //run our ADC on a faster loop
  int adcDelta = millis() - previousADCMillis;
  if (adcDelta >= YB_ADC_INTERVAL)
  {
    //this is a bit slow, so only do it once per update
    for (byte i = 0; i < CHANNEL_COUNT; i++)
      channels[i].amperage = adc_readAmperage(i);

    //check what our power is.
    busVoltage = adc_readBusVoltage();
  
    //record our total consumption
    for (byte i = 0; i < CHANNEL_COUNT; i++) {
      if (channels[i].amperage > 0)
      {
        channels[i].ampHours += channels[i].amperage * ((float)adcDelta / 3600000.0);
        channels[i].wattHours += channels[i].amperage * busVoltage * ((float)adcDelta / 3600000.0);
      }
    }

    //are our loads ok?
    for (byte id = 0; id < CHANNEL_COUNT; id++)
      channels[id].checkSoftFuse();

    // Use the snapshot to set track time until next event
    previousADCMillis = millis();
  }
}

uint16_t adc_readMCP3208Channel(byte channel, byte samples)
{
  uint32_t value = 0;

  if (samples > 1)
  {
    for (byte i = 0; i < samples; i++)
      value += adc.readADC(channel);
    value = value / samples;
  } else
    value = adc.readADC(channel);

  return (uint16_t)value;
}

float adc_readAmperage(byte channel) 
{
    uint16_t val = adc_readMCP3208Channel(channel);

    float volts = val * (3.3 / 4096.0);
    float amps = (volts - (3.3 * 0.1)) / (0.132); //ACS725LLCTR-20AU

    //our floor is zero amps
    amps = max((float)0.0, amps);

    return amps;
}

float adc_readBusVoltage()
{
  //multisample because esp32 adc is trash
  byte samples = 10;
  float busmV = 0;
  for (byte i=0; i<samples; i++)
    busmV += adc_readBusVoltageADC();
  busmV = busmV / (float)samples;

  //our resistor divider network.
  float r1 = BUS_VOLTAGE_R1;
  float r2 = BUS_VOLTAGE_R2;

  return (busmV / 1000.0) / (r2 / (r2+r1));
}

int adc_readBusVoltageADC()
{
  #ifdef BUS_VOLTAGE_ESP32
    return analogReadMilliVolts(BUS_VOLTAGE_PIN);
  #endif

  #ifdef BUS_VOLTAGE_MCP3221
  #endif
}