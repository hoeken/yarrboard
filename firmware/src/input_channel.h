/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_INPUT_CHANNEL_H
#define YARR_INPUT_CHANNEL_H

#include <Arduino.h>
#include "config.h"
#include "prefs.h"

class InputChannel
{
  byte _pins[YB_INPUT_CHANNEL_COUNT] = YB_INPUT_CHANNEL_PINS;

  public:
    byte id = 0;
    bool state = false;
    bool isEnabled = true;
    char name[YB_CHANNEL_NAME_LENGTH];

    unsigned int stateChangeCount = 0;

    bool lastState = false;

    void setup();
    void update();
};

extern InputChannel input_channels[YB_INPUT_CHANNEL_COUNT];

void input_channels_setup();
void input_channels_loop();
bool isValidInputChannel(byte cid);

#endif /* !YARR_INPUT_CHANNEL_H */