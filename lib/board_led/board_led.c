#include "board_led.h"
#include "esp_log.h"
#include "driver/gpio.h" 

static const char *TAG = "board_led";

#ifdef NEOPIXEL_PIN

led_strip_t *neopixel;

void neopixel_set(int r, int g, int b)
{
  ESP_ERROR_CHECK(neopixel->set_pixel(neopixel, 0, r, g, b));
}

void neopixel_refresh(void)
{
  ESP_ERROR_CHECK(neopixel->refresh(neopixel, 100));
}

void neopixel_init(void)
{
  rmt_config_t config = RMT_DEFAULT_CONFIG_TX(NEOPIXEL_PIN, RMT_CHANNEL);
  config.clk_div = 2; // Set clock divider, adjust as needed
  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

  led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(NEOPIXEL_NUM, (led_strip_dev_t)config.channel);
  neopixel = led_strip_new_rmt_ws2812(&strip_config);

  if (!neopixel)
  {
    ESP_LOGE(TAG, "Failed to initialize LED strip");
    return;
  }

  ESP_ERROR_CHECK(neopixel->clear(neopixel, 100));
  vTaskDelay(10 / portTICK_PERIOD_MS);
  neopixel_set( 32, 16, 0);
  neopixel_refresh();
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

#endif

#ifdef BOARD_LED_PIN

void board_led_init(void)
{
  gpio_set_direction(BOARD_LED_PIN, GPIO_MODE_OUTPUT);
}

void board_led_set(int value)
{
  gpio_set_level(BOARD_LED_PIN, value);
}

#endif
