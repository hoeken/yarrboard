#ifndef _CONFIG_H_8CH_MOSFET_REVA
#define _CONFIG_H_8CH_MOSFET_REVA

#define YB_HARDWARE_VERSION "RGB-INPUT-REV-A"

#define YB_HAS_INPUT_CHANNELS
#define YB_INPUT_CHANNEL_COUNT 16
#define YB_INPUT_CHANNEL_PINS { 13, 12, 14, 27, 26, 25, 33, 32, 35, 34, 39, 36, 2, 4, 16, 15 }
#define YB_INPUT_DEBOUNCE_RATE_MS 20

#define YB_HAS_ADC_CHANNELS
#define YB_ADC_CHANNEL_COUNT 8
#define YB_ADC_DRIVER_MCP3208
#define YB_ADC_CS 17

#define YB_HAS_RGB_CHANNELS
#define YB_RGB_CHANNEL_COUNT 16
#define YB_RGB_CHANNEL_RESOLUTION 12
#define YB_RGB_UPDATE_RATE_MS 50
#define YB_RGB_DRIVER_TLC5947
#define YB_RGB_TLC5947_NUM 2
#define YB_RGB_TLC5947_CLK 18
#define YB_RGB_TLC5947_DATA 23
#define YB_RGB_TLC5947_LATCH 15

#endif // _CONFIG_H_8CH_MOSFET_REVB