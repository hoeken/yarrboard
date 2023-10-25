/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_RGB_CHANNEL_H
#define YARR_RGB_CHANNEL_H

#include <Arduino.h>
#include "config.h"
#include "prefs.h"

class RGBChannel
{
  public:
    byte id = 0;

    bool state = false;
    float red = 0.0;
    float green = 0.0;
    float blue = 0.0;

    bool isEnabled = true;
    char name[YB_CHANNEL_NAME_LENGTH];

    void setup();
    void update();
};

extern RGBChannel rgb_channels[YB_RGB_CHANNEL_COUNT];

void rgb_channels_setup();
void rgb_channels_loop();
bool isValidRGBChannel(byte cid);

#endif /* !YARR_RGB_CHANNEL_H */