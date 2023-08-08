/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "channel.h"

//the main star of the event
OutputChannel channels[CHANNEL_COUNT];

//flag for hardware fade status
static volatile bool isChannelFading[FAN_COUNT];

/* Setting PWM Properties */
const int MAX_DUTY_CYCLE = (int)(pow(2, CHANNEL_PWM_RESOLUTION) - 1);

/*
 * This callback function will be called when fade operation has ended
 * Use callback only if you are aware it is being called inside an ISR
 * Otherwise, you can use a semaphore to unblock tasks
 */
static bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    portBASE_TYPE taskAwoken = pdFALSE;

    if (param->event == LEDC_FADE_END_EVT) {
        isChannelFading[(int)user_arg] = false;
    }

    return (taskAwoken == pdTRUE);
}

void channel_setup()
{

  //for hardware fade end callback interrupt
  ledc_fade_func_install(0);
  //intitialize our channel
  for (short i = 0; i < CHANNEL_COUNT; i++)
  {
    channels[i].id = i;
    channels[i].setup();
  }
}

void channel_loop()
{
  //maintenance on our channels.
  for (byte id = 0; id < CHANNEL_COUNT; id++)
  {
    channels[id].saveThrottledDutyCycle();
    channels[id].checkIfFadeOver();
  }
}

void OutputChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  ledc_cbs_t callbacks = {
      .fade_cb = cb_ledc_fade_end_event
  };

  //initialize our PWM channels
  ledcSetup(this->id, CHANNEL_PWM_FREQUENCY, CHANNEL_PWM_RESOLUTION);
  ledcAttachPin(this->_pins[this->id], this->id);
  ledcWrite(this->id, 0);

  //this is our callback handler for fade end.
  int channel = this->id;
  ledc_cb_register(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)this->id, &callbacks, (void *)channel);
  isChannelFading[this->id] = false;

  //lookup our name
  sprintf(prefIndex, "cName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "Channel #%d", this->id);

  //enabled or no
  sprintf(prefIndex, "cEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  //lookup our duty cycle
  sprintf(prefIndex, "cDuty%d", this->id);
  if (preferences.isKey(prefIndex))
    this->dutyCycle = preferences.getFloat(prefIndex);
  else
    this->dutyCycle = 1.0;

  //dimmability.
  sprintf(prefIndex, "cDimmable%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isDimmable = preferences.getBool(prefIndex);
  else
    this->isDimmable = true;

  //soft fuse
  sprintf(prefIndex, "cSoftFuse%d", this->id);
  if (preferences.isKey(prefIndex))
    this->softFuseAmperage = preferences.getFloat(prefIndex);
  else
    this->softFuseAmperage = YB_FAN_MAX_CHANNEL_AMPS;

  //soft fuse trip count
  sprintf(prefIndex, "cTripCount%d", this->id);
  if (preferences.isKey(prefIndex))
    this->softFuseTripCount = preferences.getUInt(prefIndex);
  else
    this->softFuseTripCount = 0;  
}

void OutputChannel::saveThrottledDutyCycle()
{
  //after 5 secs of no activity, we can save it.
  if (this->dutyCycleIsThrottled && millis() - this->lastDutyCycleUpdate > YB_DUTY_SAVE_TIMEOUT)
  {
    char prefIndex[YB_PREF_KEY_LENGTH];
    sprintf(prefIndex, "cDuty%d", this->id);
    preferences.putFloat(prefIndex, this->dutyCycle);
    this->dutyCycleIsThrottled = false;
  }
}

void OutputChannel::updateOutput()
{
  //what PWM do we want?
  int pwm = 0;
  if (this->isDimmable)
    pwm = this->dutyCycle * MAX_DUTY_CYCLE;
  else
    pwm = MAX_DUTY_CYCLE;

  //if its off, zero it out
  if (!this->state)
    pwm = 0;

  //if its tripped, zero it out.
  if (this->tripped)
    pwm = 0;

  //okay, set our pin state.
  if (!isChannelFading[this->id])
    ledcWrite(this->id, pwm);
}

void OutputChannel::checkSoftFuse()
{
  //only trip once....
  if (!this->tripped)
  {
    //Check our soft fuse, and our max limit for the board.
    if (abs(this->amperage) >= this->softFuseAmperage || abs(this->amperage) >= YB_FAN_MAX_CHANNEL_AMPS)
    {
      //actually shut it down!
      this->updateOutput();

      //record some variables
      this->tripped = true;
      this->state = false;
      this->softFuseTripCount++;

      //save to our storage
      char prefIndex[YB_PREF_KEY_LENGTH];
      sprintf(prefIndex, "cTripCount%d", this->id);
      preferences.putUInt(prefIndex, this->softFuseTripCount);
    }
  }
}

void OutputChannel::setFade(float duty, int max_fade_time_ms)
{
  // is our earlier hardware fade over yet?
  if (!isChannelFading[this->id])
  {
    //fading turns on the channel.
    this->state = true;

    //some vars for tracking.
    isChannelFading[this->id] = true;
    this->fadeRequested = true;
    this->fadeDutyCycleStart = this->dutyCycle;
    this->fadeDutyCycleEnd = duty;
    this->fadeStartTime = millis();
    this->fadeEndTime = millis() + max_fade_time_ms + 100;

    //call our hardware fader
    int target_duty = duty * MAX_DUTY_CYCLE;
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)this->id, target_duty, max_fade_time_ms, LEDC_FADE_NO_WAIT);
  }
}

void OutputChannel::checkIfFadeOver()
{
  //we're looking to see if the fade is over yet
  if (this->fadeRequested)
  {
    //has our fade ended?
    if (millis() > this->fadeEndTime)
    {
      //this is a potential bug fix.. had the board "lock" into a fade.
      //it was responsive but wouldnt toggle some pins.  I think it was this flag not getting cleared
      if (isChannelFading[this->id])
      {
        isChannelFading[this->id];
        Serial.println("error fading");
      }

      this->fadeRequested = false;
      this->setDuty(fadeDutyCycleEnd);
    }
    //okay, update our duty cycle as we go for good UI
    else
    {
      unsigned long delta = this->fadeEndTime - this->fadeStartTime;
      unsigned long nowDelta = millis() - this->fadeStartTime;
      float dutyDelta = this->fadeDutyCycleEnd - this->fadeDutyCycleStart;

      if (delta > 0)
      {
        float currentDuty = this->fadeDutyCycleStart + ((float)nowDelta / (float)delta) * dutyDelta;
        this->setDuty(currentDuty);
      }
    }

    if (this->dutyCycle == 0)
      this->state = false;
    else
      this->state = true;
  }
}

void OutputChannel::setDuty(float duty)
{
  //save to ram
  this->dutyCycle = duty;

  //save to our storage
  if (millis() - this->lastDutyCycleUpdate > YB_DUTY_SAVE_TIMEOUT)
  {
    char prefIndex[YB_PREF_KEY_LENGTH];
    sprintf(prefIndex, "cDuty%d", this->id);
    preferences.putFloat(prefIndex, duty);
    this->dutyCycleIsThrottled = false;
    Serial.printf("saving %s: %f\n", prefIndex, duty);
  }
  //make a note so we can save later.
  else
    this->dutyCycleIsThrottled = true;

  //we want the clock to reset every time we change the duty cycle
  //this way, long led fading sessions are only one write.
  this->lastDutyCycleUpdate = millis();
}