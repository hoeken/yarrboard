#ifndef _ADC_H_
#define _ADC_H_

#include <Arduino.h>
#include <MCP3208.h>

#include <Wire.h>
#include <MCP3X21.h>


void adc_setup();
uint16_t adc_readMCP3208Channel(byte channel, byte samples = 64);
float adc_readAmperage(byte channel);

float adc_readBusVoltage();
int adc_readBusVoltageADC();

#endif