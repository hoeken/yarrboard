/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include <Arduino.h>
#include "config.h"
#include "channel.h"
#include "prefs.h"

const byte outputPins[CHANNEL_COUNT] = CHANNEL_PINS;
bool channelState[CHANNEL_COUNT];
bool channelTripped[CHANNEL_COUNT];
float channelDutyCycle[CHANNEL_COUNT];
bool channelIsEnabled[CHANNEL_COUNT];
bool channelIsDimmable[CHANNEL_COUNT];
unsigned long channelLastDutyCycleUpdate[CHANNEL_COUNT];
bool channelDutyCycleIsThrottled[CHANNEL_COUNT];
float channelAmperage[CHANNEL_COUNT];
float channelSoftFuseAmperage[CHANNEL_COUNT];
float channelAmpHour[CHANNEL_COUNT];
float channelWattHour[CHANNEL_COUNT];
char channelNames[CHANNEL_COUNT][YB_CHANNEL_NAME_LENGTH];
unsigned int channelStateChangeCount[CHANNEL_COUNT];
unsigned int channelSoftFuseTripCount[CHANNEL_COUNT];

/* Setting PWM Properties */
const int MAX_DUTY_CYCLE = (int)(pow(2, CHANNEL_PWM_RESOLUTION) - 1);

void channel_setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  //intitialize our output pins.
  for (short i = 0; i < CHANNEL_COUNT; i++)
  {
    //init our storage variables.
    channelState[i] = false;
    channelTripped[i] = false;
    channelAmperage[i] = 0.0;
    channelAmpHour[i] = 0.0;
    channelWattHour[i] = 0.0;
    channelLastDutyCycleUpdate[i] = 0;
    channelDutyCycleIsThrottled[i] = false;

    //initialize our PWM channels
    //pinMode(outputPins[i], OUTPUT);
    ledcSetup(i, CHANNEL_PWM_FREQUENCY, CHANNEL_PWM_RESOLUTION);
    ledcAttachPin(outputPins[i], i);
    ledcWrite(i, 0);

    //lookup our name
    sprintf(prefIndex, "cName%d", i);
    if (preferences.isKey(prefIndex))
      strlcpy(channelNames[i], preferences.getString(prefIndex).c_str(), YB_CHANNEL_NAME_LENGTH);
    else
      sprintf(channelNames[i], "Channel #%d", i);

    //enabled or no
    sprintf(prefIndex, "cEnabled%d", i);
    if (preferences.isKey(prefIndex))
      channelIsEnabled[i] = preferences.getBool(prefIndex);
    else
      channelIsEnabled[i] = true;

    //lookup our duty cycle
    sprintf(prefIndex, "cDuty%d", i);
    if (preferences.isKey(prefIndex))
      channelDutyCycle[i] = preferences.getFloat(prefIndex);
    else
      channelDutyCycle[i] = 1.0;

    //dimmability.
    sprintf(prefIndex, "cDimmable%d", i);
    if (preferences.isKey(prefIndex))
      channelIsDimmable[i] = preferences.getBool(prefIndex);
    else
      channelIsDimmable[i] = true;

    //soft fuse
    sprintf(prefIndex, "cSoftFuse%d", i);
    if (preferences.isKey(prefIndex))
      channelSoftFuseAmperage[i] = preferences.getFloat(prefIndex);
    else
      channelSoftFuseAmperage[i] = 20.0;

    //soft fuse trip count
    sprintf(prefIndex, "cTripCount%d", i);
    if (preferences.isKey(prefIndex))
      channelSoftFuseTripCount[i] = preferences.getUInt(prefIndex);
    else
      channelSoftFuseTripCount[i] = 0;
  }
}

void channel_loop()
{
  //are any of our channels throttled?
  for (byte id = 0; id < CHANNEL_COUNT; id++)
  {
    //after 5 secs of no activity, we can save it.
    if (channelDutyCycleIsThrottled[id] && millis() - channelLastDutyCycleUpdate[id] > 5000)
    {
      char prefIndex[YB_PREF_KEY_LENGTH];
      sprintf(prefIndex, "cDuty%d", id);
      preferences.putFloat(prefIndex, channelDutyCycle[id]);
      channelDutyCycleIsThrottled[id] = false;
    }
  }
}

void updateChannelState(int channelId)
{
  //what PWM do we want?
  int pwm = 0;
  if (channelIsDimmable[channelId])
    pwm = channelDutyCycle[channelId] * MAX_DUTY_CYCLE;
  else
    pwm = MAX_DUTY_CYCLE;

  //if its off, zero it out
  if (!channelState[channelId])
    pwm = 0;

  //if its tripped, zero it out.
  if (channelTripped[channelId])
    pwm = 0;

  //okay, set our pin state.
  ledcWrite(channelId, pwm);
}

void checkSoftFuses()
{
  for (byte channel = 0; channel < CHANNEL_COUNT; channel++)
  {
    //only trip once....
    if (!channelTripped[channel])
    {
      //Check our soft fuse, and our max limit for the board.
      if (abs(channelAmperage[channel]) >= channelSoftFuseAmperage[channel] || abs(channelAmperage[channel]) >= 20.0)
      {
        //record some variables
        channelTripped[channel] = true;
        channelState[channel] = false;
        channelSoftFuseTripCount[channel]++;

        //save to our storage
        char prefIndex[YB_PREF_KEY_LENGTH];
        sprintf(prefIndex, "cTripCount%d", channel);
        preferences.putUInt(prefIndex, channelSoftFuseTripCount[channel]);

        //actually shut it down!
        updateChannelState(channel);
      }
    }
  }
}