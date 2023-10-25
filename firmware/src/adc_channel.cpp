/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_ADC

#include "adc_channel.h"

//the main star of the event
ADCChannel adc_channels[YB_ADC_CHANNEL_COUNT];

#ifdef YB_ADC_DRIVER_MCP3208
    MCP3208 _adcAnalogMCP3208;
#endif

void adc_channels_setup()
{
  #ifdef YB_ADC_DRIVER_MCP3208
    _adcAnalogMCP3208.begin(YB_ADC_CS);
  #endif

  //intitialize our channel
  for (short i = 0; i < YB_ADC_CHANNEL_COUNT; i++)
  {
    adc_channels[i].id = i;
    adc_channels[i].setup();
  }
}

void adc_channels_loop()
{
    //maintenance on our channels.
    for (byte id = 0; id < YB_ADC_CHANNEL_COUNT; id++)
        adc_channels[id].update();
}

bool isValidADCChannel(byte cid)
{
  if (cid < 0 || cid >= YB_ADC_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void ADCChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  //lookup our name
  sprintf(prefIndex, "adcName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "ADC #%d", this->id);

  //enabled or no
  sprintf(prefIndex, "adcEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  this->adcHelper = new MCP3208Helper(3.3, this->id, &_adcAnalogMCP3208);  
}

void ADCChannel::update()
{
  this->adcHelper->getReading();
}

#endif