#include "adc.h"
#include "config.h"
#include "channel.h"

//object for our adc
MCP3208 adc;
const byte adc_cs_pin = 17;

//for watching our power supply
float busVoltage = 0;

//for tracking our ADC loop
int adcInterval = 100;
unsigned long previousADCMillis = 0;

#ifdef BUS_VOLTAGE_MCP3221
  MCP3221 mcp3221(BUS_VOLTAGE_ADDRESS);
#endif

void adc_setup()
{
  adc.begin(adc_cs_pin);

  #ifdef BUS_VOLTAGE_ESP32
    adcAttachPin(BUS_VOLTAGE_PIN);
    analogSetAttenuation(ADC_11db);
  #endif

  #ifdef BUS_VOLTAGE_MCP3221
    mcp3221.init();
  #endif
}

void adc_loop()
{
  //run our ADC on a faster loop
  int adcDelta = millis() - previousADCMillis;
  if (adcDelta >= adcInterval)
  {
    //this is a bit slow, so only do it once per update
    for (byte channel = 0; channel < CHANNEL_COUNT; channel++)
      channelAmperage[channel] = adc_readAmperage(channel);

    //check what our power is.
    busVoltage = adc_readBusVoltage();
  
    //record our total consumption
    for (byte i = 0; i < CHANNEL_COUNT; i++) {
      if (channelAmperage[i] > 0)
      {
        channelAmpHour[i] += channelAmperage[i] * ((float)adcDelta / 3600000.0);
        channelWattHour[i] += channelAmperage[i] * busVoltage * ((float)adcDelta / 3600000.0);
      }
    }

    //are our loads ok?
    checkSoftFuses();

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
    return mcp3221.read();
  #endif
}