/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BUS_VOLTAGE_H
#define YARR_BUS_VOLTAGE_H

#include <Arduino.h>
#include "config.h"
#include "prefs.h"
#include "adchelper.h"

extern float busVoltage;
float getBusVoltage();

void bus_voltage_setup();
void bus_voltage_loop();

#endif /* !YARR_BUS_VOLTAGE_H */