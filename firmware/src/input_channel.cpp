/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_DIGITAL_INPUT_CHANNELS

#include "input_channel.h"

//the main star of the event
InputChannel input_channels[YB_INPUT_CHANNEL_COUNT];

unsigned long lastInputCheckMillis = 0;

void input_channels_setup()
{
  //intitialize our channel
  for (short i = 0; i < YB_INPUT_CHANNEL_COUNT; i++)
  {
    input_channels[i].id = i;
    input_channels[i].setup();
  }
}

void input_channels_loop()
{
  if (millis() > lastInputCheckMillis + YB_DEBOUNCE_RATE_MS)
  {
    //maintenance on our channels.
    for (byte id = 0; id < YB_INPUT_CHANNEL_COUNT; id++)
        input_channels[id].update();

    lastInputCheckMillis = millis();
  }
}

bool isValidInputChannel(byte cid)
{
  if (cid < 0 || cid >= YB_INPUT_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void InputChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  //lookup our name
  sprintf(prefIndex, "iptName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "Switch #%d", this->id);

  //enabled or no
  sprintf(prefIndex, "iptEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  //setup our pin
  pinMode(this->_pins[this->id], INPUT);

  //initial values
  this->state = digitalRead(this->_pins[this->id]);
  this->lastState = this->state;
  this->stateChangeCount = 0;
}

void InputChannel::update()
{
    bool currentState = digitalRead(this->_pins[this->id]);

    if (currentState == this->lastState && currentState != this->state)
    {
        this->state = this->lastState;
        this->stateChangeCount++;
    }

    this->lastState = currentState;
}

#endif