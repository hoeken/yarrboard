#include <Arduino.h>

void adc_setup();
uint16_t adc_readMCP3208Channel(byte channel, byte samples = 64);
float adc_readAmperage(byte channel);
float adc_readBusVoltage();