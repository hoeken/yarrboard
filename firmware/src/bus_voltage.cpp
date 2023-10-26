/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_BUS_VOLTAGE

#include "bus_voltage.h"

//for watching our power supply
float busVoltage = 0;

#ifdef YB_BUS_VOLTAGE_ESP32
  esp32Helper busADC = esp32Helper(3.3, YB_BUS_VOLTAGE_PIN);
#endif

#ifdef YB_BUS_VOLTAGE_MCP3425
  MCP342x _adcMCP3425 = MCP342x(YB_BUS_VOLTAGE_ADDRESS);
  MCP342x::Config config(MCP342x::channel1, MCP342x::oneShot, MCP342x::resolution18, MCP342x::gain1);

  MCP3425Helper busADC = MCP3425Helper((float)3.3, &_adcMCP3425);
#endif

void bus_voltage_setup()
{
  #ifdef YB_BUS_VOLTAGE_ESP32
    adcAttachPin(YB_BUS_VOLTAGE_PIN);
    analogSetAttenuation(ADC_11db);
  #endif

  #ifdef YB_BUS_VOLTAGE_MCP3425
    Wire.begin();
    MCP342x::generalCallReset();
    delay(1); // MC342x needs 300us to settle, wait 1ms

    //check if its there...
    Wire.requestFrom(YB_BUS_VOLTAGE_ADDRESS, 1);
    if (!Wire.available())
      Serial.println("ERROR: MCP3425 Not found.");

    _adcMCP3425.configure(config);
    MCP342x::generalCallConversion();
  #endif
}

void bus_voltage_loop()
{
    //check what our power is.
    busADC.getReading();
}

float getBusVoltage()
{
    float voltage = busADC.getAverageVoltage();

    busADC.resetAverage();

    return voltage / (YB_BUS_VOLTAGE_R2 / (YB_BUS_VOLTAGE_R2+YB_BUS_VOLTAGE_R1));
}

#endif