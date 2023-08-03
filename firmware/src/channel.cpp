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
String channelNames[CHANNEL_COUNT];
unsigned int channelStateChangeCount[CHANNEL_COUNT];
unsigned int channelSoftFuseTripCount[CHANNEL_COUNT];

/* Setting PWM Properties */
//const int PWMFreq = 5000; /* in Hz  */
const int PWMResolution = 8;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

void channel_setup()
{
  String prefIndex;

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
    pinMode(outputPins[i], OUTPUT);
    analogWrite(outputPins[i], 0);

    //lookup our name
    prefIndex = "cName" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelNames[i] = preferences.getString(prefIndex.c_str());
    else
      channelNames[i] = "Channel #" + String(i);

    //enabled or no
    prefIndex = "cEnabled" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelIsEnabled[i] = preferences.getBool(prefIndex.c_str());
    else
      channelIsEnabled[i] = true;

    //lookup our duty cycle
    prefIndex = "cDuty" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelDutyCycle[i] = preferences.getFloat(prefIndex.c_str());
    else
      channelDutyCycle[i] = 1.0;

    //dimmability.
    prefIndex = "cDimmable" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelIsDimmable[i] = preferences.getBool(prefIndex.c_str());
    else
      channelIsDimmable[i] = true;

    //soft fuse
    prefIndex = "cSoftFuse" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelSoftFuseAmperage[i] = preferences.getFloat(prefIndex.c_str());
    else
      channelSoftFuseAmperage[i] = 20.0;

    //soft fuse trip count
    prefIndex = "cTripCount" + String(i);
    if (preferences.isKey(prefIndex.c_str()))
      channelSoftFuseTripCount[i] = preferences.getUInt(prefIndex.c_str());
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
      String prefIndex = "cDuty" + String(id);
      preferences.putFloat(prefIndex.c_str(), channelDutyCycle[id]);
      channelDutyCycleIsThrottled[id] = false;
    }
  }
}

void updateChannelState(int channelId)
{
  //what PWM do we want?
  int pwm = 0;
  if (channelIsDimmable[channelId])
    pwm = (int)(channelDutyCycle[channelId] * MAX_DUTY_CYCLE);
  else
    pwm = MAX_DUTY_CYCLE;

  //if its tripped, zero it out.
  if (channelTripped[channelId])
    pwm = 0;

  //okay, set our pin state.
  if (channelState[channelId])
    analogWrite(outputPins[channelId], pwm);
  else
    analogWrite(outputPins[channelId], 0);
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
        String prefIndex = "cTripCount" + String(channel);
        preferences.putUInt(prefIndex.c_str(), channelSoftFuseTripCount[channel]);

        //actually shut it down!
        updateChannelState(channel);
      }
    }
  }
}