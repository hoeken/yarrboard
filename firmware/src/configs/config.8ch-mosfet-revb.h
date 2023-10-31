#ifndef _CONFIG_H_8CH_MOSFET_REVB
#define _CONFIG_H_8CH_MOSFET_REVB

#define YB_HARDWARE_VERSION "8CH_MOSFET_REV_B"

#define YB_HAS_PWM_CHANNELS
#define YB_PWM_CHANNEL_COUNT 8
#define YB_PWM_CHANNEL_PINS { 32, 33, 25, 26, 27, 14, 12, 13 }
#define YB_PWM_CHANNEL_FREQUENCY 1000
#define YB_PWM_CHANNEL_RESOLUTION 10
#define YB_PWM_CHANNEL_ADC_DRIVER_MCP3208
#define YB_PWM_CHANNEL_ADC_CS 17
#define YB_PWM_CHANNEL_MAX_AMPS 20.0
//#define YB_PWM_CHANNEL_TOTAL_AMPS 100.0

#define YB_HAS_BUS_VOLTAGE
#define YB_BUS_VOLTAGE_MCP3425
#define YB_BUS_VOLTAGE_R1 300000.0
#define YB_BUS_VOLTAGE_R2 22000.0
#define YB_BUS_VOLTAGE_ADDRESS 0x68

#define YB_HAS_FANS
#define YB_FAN_COUNT 2
#define YB_FAN_PWM_PIN 16
//NOTE: ESP32 Errata 3.14. Within the same group of GPIO pins, edge interrupts cannot be used together with other interrupts.
#define YB_FAN_TACH_PINS {39, 4}
#define YB_FAN_SINGLE_CHANNEL_THRESHOLD_START 5
#define YB_FAN_SINGLE_CHANNEL_THRESHOLD_END 10
#define YB_FAN_AVERAGE_CHANNEL_THRESHOLD_START 2
#define YB_FAN_AVERAGE_CHANNEL_THRESHOLD_END 4

#endif // _CONFIG_H_8CH_MOSFET_REVB