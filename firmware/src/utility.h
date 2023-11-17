/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_UTILITY_H
#define YARR_UTILITY_H

#include "config.h"

double round2(double value);
double round3(double value);
double round4(double value);

extern unsigned int framerate;
extern int ticklist[YB_FPS_SAMPLES];

double calculateFramerate(int newtick);

#endif /* !YARR_UTILITY_H */