/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#include <SPI.h>

#ifdef YB_HAS_RGB_CHANNELS

#include "rgb_channel.h"

#ifdef YB_CONFIG_RGB_INPUT_REV_A
  SPIClass mySPI(VSPI);
#else
  SPIClass mySPI(HSPI);
#endif

#ifdef YB_RGB_DRIVER_TLC5947
  TLC5947_SPI tlc = TLC5947_SPI(YB_RGB_TLC5947_NUM, YB_RGB_TLC5947_LATCH, &mySPI);
#endif

//the main star of the event
RGBChannel rgb_channels[YB_RGB_CHANNEL_COUNT];

const int MAX_RGB_RESOLUTION = (int)(pow(2, YB_RGB_CHANNEL_RESOLUTION) - 1);

unsigned long lastRGBUpdateMillis = 0;
bool rgb_is_dirty = false;

void rgb_channels_setup()
{
  #ifdef YB_RGB_TLC5947_BLANK
    pinMode(YB_RGB_TLC5947_BLANK, OUTPUT);
    digitalWrite(YB_RGB_TLC5947_BLANK, HIGH);
    //digitalWrite(YB_RGB_TLC5947_BLANK, LOW);
  #endif

  #ifdef YB_RGB_DRIVER_TLC5947
    tlc.begin();
  #endif

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
    if (rgb_is_dirty)
    {
       #ifdef YB_RGB_DRIVER_TLC5947
        #ifdef YB_RGB_TLC5947_BLANK
          digitalWrite(YB_RGB_TLC5947_BLANK, HIGH);
        #endif

        tlc.write();

        #ifdef YB_RGB_TLC5947_BLANK
          digitalWrite(YB_RGB_TLC5947_BLANK, LOW);
        #endif
      #endif

      rgb_is_dirty = false;
    }

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

  this->setRGB(0.0, 0.0, 0.0);
}

void RGBChannel::setRGB(float r, float g, float b)
{
  this->red = r;
  this->green = g;
  this->blue = b;

  #ifdef YB_RGB_DRIVER_TLC5947
    tlc.setLED(this->id, this->red*MAX_RGB_RESOLUTION, this->green*MAX_RGB_RESOLUTION, this->blue*MAX_RGB_RESOLUTION);
  #endif

  rgb_is_dirty = true;
}

#endif