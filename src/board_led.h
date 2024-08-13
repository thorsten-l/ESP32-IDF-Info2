#pragma once

#ifdef NEOPIXEL_PIN

#define CONFIG_RMT_SUPPRESS_DEPRECATE_WARN 1
#define NEOPIXEL_NUM 1
#define RMT_CHANNEL RMT_CHANNEL_0

#include <driver/rmt.h>
#include <led_strip.h>

extern led_strip_t *neopixel;

extern void neopixel_init(void);
extern void neopixel_set(int r, int g, int b);
extern void neopixel_refresh(void);

#define NEOPIXEL_INIT() neopixel_init()
#define NEOPIXEL_SET(r, g, b) neopixel_set(r, g, b)
#define NEOPIXEL_REFRESH() neopixel_refresh()

#else

#define NEOPIXEL_INIT()
#define NEOPIXEL_SET(r, g, b)
#define NEOPIXEL_REFRESH()

#endif


#ifdef BOARD_LED_PIN

  extern void board_led_init(void);
  extern void board_led_set(int value);

  #define BOARD_LED_INIT() board_led_init()
  #define BOARD_LED_SET(value) board_led_set(value)

#else

  #define BOARD_LED_INIT()
  #define BOARD_LED_SET(value)

#endif
