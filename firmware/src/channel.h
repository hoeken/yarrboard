/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_CHANNEL_H
#define YARR_CHANNEL_H

#include <Arduino.h>
#include "config.h"
#include "driver/ledc.h"

//keep track of our channel info.
extern const byte outputPins[CHANNEL_COUNT];
extern bool channelState[CHANNEL_COUNT];
extern bool channelTripped[CHANNEL_COUNT];
extern float channelDutyCycle[CHANNEL_COUNT];
extern bool channelIsEnabled[CHANNEL_COUNT];
extern bool channelIsDimmable[CHANNEL_COUNT];
extern unsigned long channelLastDutyCycleUpdate[CHANNEL_COUNT];
extern bool channelDutyCycleIsThrottled[CHANNEL_COUNT];
extern float channelAmperage[CHANNEL_COUNT];
extern float channelSoftFuseAmperage[CHANNEL_COUNT];
extern float channelAmpHour[CHANNEL_COUNT];
extern float channelWattHour[CHANNEL_COUNT];
extern char channelNames[CHANNEL_COUNT][YB_CHANNEL_NAME_LENGTH];
extern unsigned int channelStateChangeCount[CHANNEL_COUNT];
extern unsigned int channelSoftFuseTripCount[CHANNEL_COUNT];

void channel_setup();
void channel_loop();
void updateChannelState(int channelId);
void checkSoftFuses();
void channelFade(uint8_t channel, float duty, int fadeTime);

#endif /* !YARR_CHANNEL_H */