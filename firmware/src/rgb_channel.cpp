/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_RGB_OUTPUT

#include "rgb_channel.h"

//the main star of the event
RGBChannel rgb_channels[YB_RGB_CHANNEL_COUNT];

unsigned long lastRGBUpdateMillis = 0;

void rgb_channels_setup()
{
  //intitialize our channel
  for (short i = 0; i < YB_RGB_CHANNEL_COUNT; i++)
  {
    rgb_channels[i].id = i;
    rgb_channels[i].setup();
  }
}

void rgb_channels_loop()
{
  if (millis() > lastRGBUpdateMillis + YB_RGB_UPDATE_RATE_MS)
  {
    //maintenance on our channels.
    for (byte id = 0; id < YB_INPUT_CHANNEL_COUNT; id++)
        rgb_channels[id].update();

    lastRGBUpdateMillis = millis();
  }
}

bool isValidRGBChannel(byte cid)
{
  if (cid < 0 || cid >= YB_INPUT_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void RGBChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  //lookup our name
  sprintf(prefIndex, "rgbName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "RGB #%d", this->id);

  //enabled or no
  sprintf(prefIndex, "rgbEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  sprintf(prefIndex, "rgbRed%d", this->id);
  if (preferences.isKey(prefIndex))
    this->red = preferences.getFloat(prefIndex);
  else
    this->red = 0.0;

  sprintf(prefIndex, "rgbGrn%d", this->id);
  if (preferences.isKey(prefIndex))
    this->green = preferences.getFloat(prefIndex);
  else
    this->green = 0.0;

  sprintf(prefIndex, "rgbBlu%d", this->id);
  if (preferences.isKey(prefIndex))
    this->blue = preferences.getFloat(prefIndex);
  else
    this->blue = 0.0;

  this->state = false;
  this->red = 0;
  this->blue = 0;
  this->green = 0;
}

void RGBChannel::update()
{
}

#endif