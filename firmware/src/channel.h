/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_CHANNEL_H
#define YARR_CHANNEL_H

#include <Arduino.h>
#include "prefs.h"
#include "config.h"
#include "driver/ledc.h"

class OutputChannel
{
  byte _pins[CHANNEL_COUNT] = CHANNEL_PINS;

  public:
    byte id = 0;
    bool state = false;
    bool tripped = false;
    char name[YB_CHANNEL_NAME_LENGTH];

    unsigned int stateChangeCount = 0;
    unsigned int softFuseTripCount = 0;

    float dutyCycle = 0.0;
    unsigned long lastDutyCycleUpdate = 0;
    unsigned long dutyCycleIsThrottled = 0;

    bool fadeRequested = true;
    unsigned long fadeStartTime = 0;
    unsigned long fadeEndTime = 0;
    float fadeDutyCycleStart = 0;
    float fadeDutyCycleEnd = 0;

    float amperage = 0.0;
    float softFuseAmperage = 0.0;
    float ampHours = 0.0;
    float wattHours = 0.0;

    bool isEnabled = false;
    bool isDimmable = false;

    void setup();
    void saveThrottledDutyCycle();
    void updateOutput();
    void checkSoftFuse();
    void checkIfFadeOver();
    void setFade(float duty, int max_fade_time_ms);
    void setDuty(float duty);
};

extern OutputChannel channels[CHANNEL_COUNT];

//keep track of our channel info.
/*
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
*/

void channel_setup();
void channel_loop();
void checkSoftFuses();

#endif /* !YARR_CHANNEL_H */