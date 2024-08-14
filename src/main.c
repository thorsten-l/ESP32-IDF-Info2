#include <stdio.h>

#include <time.h>
#include <sys/time.h>

#include <freertos/FreeRTOS.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <spi_flash_mmap.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_app_desc.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include <esp_psram.h>

#include <config.h>
#include "app.h"
#include "board_led.h"
#include "board_sdcard.h"

static const char *TAG = "app_main";
static uint32_t counter = 0;

void secondTask(void *parameter)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  printf("Second task running core   : %d\n", xPortGetCoreID());
  xSemaphoreGive(mutex);
  vTaskDelete(NULL);
}

void loop(void *parameter)
{
  struct tm appTimeinfo;
  time_t now;
  puts("*** loop started ***");

  while (true)
  {
    delay(1000);

    if (setup_finished)
    {
      counter++;
      now = time(NULL);
      localtime_r(&now, &appTimeinfo);
      printf("\r> %d-%02d-%02d %02d:%02d:%02d",
             appTimeinfo.tm_year + 1900, appTimeinfo.tm_mon + 1, appTimeinfo.tm_mday,
             appTimeinfo.tm_hour, appTimeinfo.tm_min, appTimeinfo.tm_sec);
      fflush(stdout);        // flush the stdout buffer
      fsync(fileno(stdout)); // flush UART buffer
      NEOPIXEL_SET(0, 96 * (1 - (counter % 2)), 128 * (counter % 2));
      NEOPIXEL_REFRESH();
      BOARD_LED_SET(counter % 2);
    }
  }
}

void app_main()
{
  BOARD_LED_INIT();
  NEOPIXEL_INIT();
  nvs_flash_init();
  mutex = xSemaphoreCreateMutex();
  setup_finished = false;
  const esp_app_desc_t *app_desc = esp_app_get_description();

  delay(3000); // wait for serial monitor

  printf("\n\nESP32 Chip Info - IDF - %s by Dr. Thorsten Ludewig\n", app_desc->version);
  puts("Build date: " __DATE__ " " __TIME__ "\n");

  printf("esp idf version            : %s\n", esp_get_idf_version());
  printf("esp idf target             : %s\n", CONFIG_IDF_TARGET);
  printf("esp idf target arch        : %s\n\n", CONFIG_IDF_TARGET_ARCH);

  printf("PIO Environment            : %s\n", PIOENV);
  printf("PIO Platform               : %s\n", PIOPLATFORM);
  printf("PIO Framework              : %s\n\n", PIOFRAMEWORK);

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  uint32_t flash_size;
  esp_flash_get_size(NULL, &flash_size);

  printf("chip # of cores            : %d\n", chip_info.cores);
  printf("chip revision              : %d\n", chip_info.revision);
  printf("chip feature bt            : %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "true" : "false");
  printf("chip feature ble           : %s\n", (chip_info.features & CHIP_FEATURE_BLE) ? "true" : "false");
  printf("chip feature embed. flash  : %s\n", (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "true" : "false");
  printf("chip feature embed. psram  : %s\n", (chip_info.features & CHIP_FEATURE_EMB_PSRAM) ? "true" : "false");
  printf("chip feature IEEE 802.15.4 : %s\n", (chip_info.features & CHIP_FEATURE_IEEE802154) ? "true" : "false");
  printf("chip feature wifi bgn      : %s\n\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "true" : "false");

  printf("flash size                 : %ldMB (%s)\n", flash_size / (1024 * 1024),
         (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

#ifdef CONFIG_SPIRAM
  if (esp_psram_is_initialized())
  {
    size_t psram_size = esp_psram_get_size();
    printf("PSRAM size                 : %uMB\n", psram_size / (1024 * 1024));
  }
  else
  {
    printf("PSRAM                      : configured but not available\n");
  }
#else
  printf("PSRAM                      : not available\n");
#endif

  printf("free heap size             : %ld\n\n", esp_get_free_heap_size());
  printf("Running core               : %d\n", xPortGetCoreID());

  // -------------------------------------------------------------

  if (chip_info.cores > 1)
  {
    xTaskCreatePinnedToCore(&secondTask, "secondTask", 2048, NULL, 1, NULL, 1);
  }
  else
  {
    xTaskCreatePinnedToCore(&secondTask, "secondTask", 2048, NULL, 1, NULL, 0);
  }

  delay(50);
  xSemaphoreTake(mutex, portMAX_DELAY);
  // -------------------------------------------------------------
  puts("");
  xSemaphoreGive(mutex);
  delay(1000);

  LIST_SDCARD_ROOT();
  wifi_init_sta();
  xTaskCreatePinnedToCore(&loop, "loop", 2048, NULL, 1, NULL, 0);

  ESP_LOGI(TAG, "*** APP_MAIN finished ***");
}
