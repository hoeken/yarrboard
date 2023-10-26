/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_RGB_OUTPUT

#include "rgb_channel.h"

#ifdef YB_RGB_DRIVER_TLC5947
  Adafruit_TLC5947 tlc = Adafruit_TLC5947(YB_RGB_TLC5947_NUM, YB_RGB_TLC5947_CLK, YB_RGB_TLC5947_DATA, YB_RGB_TLC5947_LATCH);
#endif

//the main star of the event
RGBChannel rgb_channels[YB_RGB_CHANNEL_COUNT];

const int MAX_RGB_RESOLUTION = (int)(pow(2, YB_RGB_CHANNEL_RESOLUTION) - 1);

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
  //only update every so often
  if (millis() > lastRGBUpdateMillis + YB_RGB_UPDATE_RATE_MS)
  {
    tlc.write();
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

  this->setRGB(this->red, this->green, this->blue);
}

void RGBChannel::setRGB(float red, float green, float blue)
{
  this->red = red;
  this->green = green;
  this->blue = blue;

  #ifdef YB_RGB_DRIVER_TLC5947
    tlc.setLED(this->id, this->red*MAX_RGB_RESOLUTION, this->green*MAX_RGB_RESOLUTION, this->blue*MAX_RGB_RESOLUTION);
  #endif
}

#endif