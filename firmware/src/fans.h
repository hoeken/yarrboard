#ifndef YARR_FANS_H
#define YARR_FANS_H

extern int fans_last_rpm[FAN_COUNT];
extern int fans_last_pwm[FAN_COUNT];

void fans_setup();
void fans_loop();
void measure_fan_rpm(byte i);
void set_fan_pwm(byte i, byte pwm);

#endif /* !YARR_FANS_H */