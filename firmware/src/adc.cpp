#include "adc.h"

//object for our adc
MCP3208 adc;
const byte adc_cs_pin = 17;
const byte busVoltagePin = 36;

const uint8_t address = 0x4D;
const uint16_t ref_voltage = 3300;  // in mV
MCP3221 mcp3221(address);

void adc_setup()
{
  adc.begin(adc_cs_pin);

  //adc for our bus voltage.
  adcAttachPin(busVoltagePin);
  analogSetAttenuation(ADC_11db);

  mcp3221.init();
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
    busmV += analogReadMilliVolts(busVoltagePin);
  busmV = busmV / (float)samples;

  //our resistor divider network.
  float r1 = 100000.0;
  float r2 = 10000.0;
  return (busmV / 1000.0) / (r2 / (r2+r1));

  /*
  //multisample because esp32 adc is trash
  byte samples = 10;
  float busmV = 0;
  for (byte i=0; i<samples; i++)
    busmV += mcp3221.read();
  busmV = busmV / (float)samples;

  //our resistor divider network.
  float r1 = 130000.0;
  float r2 = 16000.0;

  return (busmV / 1000.0) / (r2 / (r2+r1));
  */
}