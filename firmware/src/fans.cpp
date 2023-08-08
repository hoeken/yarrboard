/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include <Arduino.h>
#include "config.h"
#include "fans.h"
#include "channel.h"

//Lots of code borrowed from: https://github.com/KlausMu/esp32-fan-controller

byte fan_pwm_pins[FAN_COUNT] = FAN_PWM_PINS;
byte fan_tach_pins[FAN_COUNT] = FAN_TACH_PINS;

static volatile int counter_rpm[FAN_COUNT];
unsigned long last_tacho_measurement[FAN_COUNT];

int fans_last_rpm[FAN_COUNT];
int fans_last_pwm[FAN_COUNT];

// Interrupt counting every rotation of the fan
// https://desire.giesecke.tk/index.php/2018/01/30/change-global-variables-from-isr/
void IRAM_ATTR rpm_fan_0() {
  counter_rpm[0]++;
}

void IRAM_ATTR rpm_fan_1() {
  counter_rpm[1]++;
}

void fans_setup()
{
    for (byte i=0; i<FAN_COUNT; i++)
    {
        ledcSetup(CHANNEL_COUNT+i, 25000, 8);
        ledcAttachPin(fan_pwm_pins[i], CHANNEL_COUNT+i);
        set_fan_pwm(i, 0);
    }

    for (byte i=0; i<FAN_COUNT; i++)
    {
        counter_rpm[i] = 0;
        last_tacho_measurement[i] = 0;
        fans_last_rpm[i] = 0;

        pinMode(fan_tach_pins[i], INPUT);
        //digitalWrite(fan_tach_pins[i], HIGH);

        if (i==0)
            attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_0, FALLING);
        if (i==1)
            attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_1, FALLING);
    }
}

void fans_loop()
{
    float amps_avg = 0;
    float amps_max = 0;
    for (byte id=0; id<CHANNEL_COUNT; id++)
    {
        amps_avg += channels[id].amperage;
        amps_max = max(amps_max, channels[id].amperage);
    }
    amps_avg = amps_avg / CHANNEL_COUNT;

    for (byte i=0; i<FAN_COUNT; i++)
    {
        if ((unsigned long)(millis() - last_tacho_measurement[i]) >= 1000)
        { 
            //get our rpm numbers
            measure_fan_rpm(i);

            //one channel on high?
            if (amps_max > YB_FAN_SINGLE_CHANNEL_AMPS)
            {
                set_fan_pwm(i, 255);
                Serial.println("Fan Full Blast");
            }
            //high average amps?
            else if (amps_avg > YB_FAN_AVERAGE_CHANNEL_AMPS)
            {
                byte pwm = map(amps_avg, YB_FAN_AVERAGE_CHANNEL_AMPS, YB_FAN_MAX_CHANNEL_AMPS, 0, 255);
                pwm = constrain(pwm, 0, 255);
                set_fan_pwm(i, pwm);

                Serial.print("Fans partial: ");
                Serial.println(pwm);
            }
            //no need to make noise
            else
                set_fan_pwm(i, 0);
        }
    }
}

void measure_fan_rpm(byte i)
{
    // detach interrupt while calculating rpm
    detachInterrupt(digitalPinToInterrupt(fan_tach_pins[i])); 

    // calculate rpm
    fans_last_rpm[i] = counter_rpm[i] * 30;
    //Serial.print("fan rpm = ");
    //Serial.println(last_rpm[i]);

    // reset counter
    counter_rpm[i] = 0;

    // store milliseconds when tacho was measured the last time
    last_tacho_measurement[i] = millis();

    // attach interrupt again
    if (i==0)
        attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_0, FALLING);
    if (i==1)
        attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_1, FALLING);
}

void set_fan_pwm(byte i, byte pwm)
{
    fans_last_pwm[i] = pwm;
    ledcWrite(CHANNEL_COUNT+i, pwm);
}