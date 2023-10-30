/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_FANS

#include <Arduino.h>
#include "fans.h"
#include "pwm_channel.h"

//Lots of code borrowed from: https://github.com/KlausMu/esp32-fan-controller

byte fan_tach_pins[YB_FAN_COUNT] = YB_FAN_TACH_PINS;
//use the pwm channel directly after our PWM channels
byte fan_pwm_channel = YB_PWM_CHANNEL_COUNT;

static volatile int counter_rpm[YB_FAN_COUNT];
unsigned long last_tacho_measurement[YB_FAN_COUNT];

int fans_last_rpm[YB_FAN_COUNT];

unsigned long lastFanCheckMillis = 0;

// Interrupt counting every rotation of the fan
// https://desire.giesecke.tk/index.php/2018/01/30/change-global-variables-from-isr/
// This is really janky because of this ESP32 bug:
// ESP32 Errata 3.14. Within the same group of GPIO pins, edge interrupts cannot be used together with other interrupts.
void IRAM_ATTR rpm_fan_0_low() {
  counter_rpm[0]++;
}
void IRAM_ATTR rpm_fan_1_low() {
  counter_rpm[1]++;
}

void fans_setup()
{
    //use the pwm channel directly after our PWM channels
    ledcSetup(fan_pwm_channel, 25000, 8);
    ledcAttachPin(YB_FAN_PWM_PIN, fan_pwm_channel);
    set_fan_pwm(0);

    for (byte i=0; i<YB_FAN_COUNT; i++)
    {
        counter_rpm[i] = 0;
        last_tacho_measurement[i] = 0;
        fans_last_rpm[i] = 0;

        pinMode(fan_tach_pins[i], INPUT);
        //digitalWrite(fan_tach_pins[i], HIGH);

        if (i==0)
            attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_0_low, FALLING);
        if (i==1)
            attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_1_low, FALLING);
    }
}

//unsigned long lastFanCheckMillis = 0;

void fans_loop()
{
    //get our rpm numbers
    for (byte i=0; i<YB_FAN_COUNT; i++)
        measure_fan_rpm(i);

    //get our current averages
    if ((unsigned long)(millis() - lastFanCheckMillis) >= 1000)
    {
        //calculate our amperages
        float amps_avg = 0;
        float amps_max = 0;
        byte enabled_count = 0;
        for (byte id=0; id<YB_PWM_CHANNEL_COUNT; id++)
        {
            //only count enabled channels
            if (pwm_channels[id].isEnabled)
            {
                amps_avg += pwm_channels[id].amperage;
                amps_max = max(amps_max, pwm_channels[id].amperage);
                enabled_count++;
            }
        }
        amps_avg = amps_avg / enabled_count;

        //one channel on high?
        if (amps_max > YB_FAN_SINGLE_CHANNEL_THRESHOLD_END)
        {
            set_fan_pwm(255);
            //Serial.println("Single channel full blast");
        }
        else if (amps_avg > YB_FAN_AVERAGE_CHANNEL_THRESHOLD_END)
        {
            set_fan_pwm(255);
            //Serial.println("Average current full blast");
        }
        //high average amps?
        else if (amps_avg > YB_FAN_AVERAGE_CHANNEL_THRESHOLD_START || amps_max > YB_FAN_SINGLE_CHANNEL_THRESHOLD_START)
        {
            int avgpwm = map_float(amps_avg, YB_FAN_AVERAGE_CHANNEL_THRESHOLD_START, YB_FAN_AVERAGE_CHANNEL_THRESHOLD_END, 0, 255);
            // Serial.print("Average pwm: ");
            // Serial.println(avgpwm);

            int singlepwm = map_float(amps_max, YB_FAN_SINGLE_CHANNEL_THRESHOLD_START, YB_FAN_SINGLE_CHANNEL_THRESHOLD_END, 0, 255);
            // Serial.print("Single pwm: ");
            // Serial.println(singlepwm);

            int pwm = max(avgpwm, singlepwm);
            pwm = constrain(pwm, 0, 255);
            // Serial.print("Fans partial: ");
            // Serial.println(pwm);

            set_fan_pwm(pwm);
        }
        //no need to make noise
        else
            set_fan_pwm(0);

        lastFanCheckMillis = millis();
    }
}

float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void measure_fan_rpm(byte i)
{
    if ((unsigned long)(millis() - last_tacho_measurement[i]) >= 1000)
    {
        // detach interrupt while calculating rpm
        detachInterrupt(digitalPinToInterrupt(fan_tach_pins[i])); 

        // calculate rpm
        fans_last_rpm[i] = counter_rpm[i] * 30;

        // reset counter
        counter_rpm[i] = 0;

        // store milliseconds when tacho was measured the last time
        last_tacho_measurement[i] = millis();

        // attach interrupt again
        if (i==0)
            attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_0_low, FALLING);
        if (i==1)
            attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_1_low, FALLING);
    }
}

void set_fan_pwm(byte pwm)
{
    ledcWrite(fan_pwm_channel, pwm);
}

#endif