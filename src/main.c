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

#include <config.h>

static const char *TAG = "app";
SemaphoreHandle_t mutex;

static void setup_sntp(void)
{
  setenv("TZ", TIMEZONE, 1);
  tzset();

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, NTP_SERVER);
  esp_sntp_init();

  //-------------------------------------------------------------

  struct tm appTimeinfo;
  time_t now;
  time_t appStartTime = 0;

  // Wait for time to be set
  puts("\n*** setup SNTP time ... (20s timeout)");
  for (int i = 0; i < 20; ++i)
  {
    time(&now);
    localtime_r(&now, &appTimeinfo);
    if (appTimeinfo.tm_year > (2023 - 1900))
    {
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  now = time(NULL);

  // Check if time was set
  if (appTimeinfo.tm_year > (2023 - 1900))
  {
    appStartTime = now - (esp_log_timestamp() / 1000);
    localtime_r(&appStartTime, &appTimeinfo);
    printf("app start time             : %d-%02d-%02d %02d:%02d:%02d\n", appTimeinfo.tm_year + 1900, appTimeinfo.tm_mon + 1, appTimeinfo.tm_mday, appTimeinfo.tm_hour, appTimeinfo.tm_min, appTimeinfo.tm_sec);
  }
  else
  {
    puts("time not set");
  }

  localtime_r(&now, &appTimeinfo);
  printf("time                       : %d-%02d-%02d %02d:%02d:%02d\n", appTimeinfo.tm_year + 1900, appTimeinfo.tm_mon + 1, appTimeinfo.tm_mday, appTimeinfo.tm_hour, appTimeinfo.tm_min, appTimeinfo.tm_sec);
  printf("timezone                   : %s\n", getenv("TZ"));

  //-------------------------------------------------------------
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    esp_wifi_connect();
    puts("\n--- retry to connect to the AP");
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    puts("\n*** IP_EVENT_STA_GOT_IP");
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    printf("ipaddr                     : " IPSTR "\n", IP2STR(&event->ip_info.ip));
    printf("gateway                    : " IPSTR "\n", IP2STR(&event->ip_info.gw));
    printf("netmask                    : " IPSTR "\n", IP2STR(&event->ip_info.netmask));

    esp_netif_dns_info_t dns_info;
    if (esp_netif_get_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK)
    {
      printf("DNS server                 : " IPSTR "\n", IP2STR(&dns_info.ip.u_addr.ip4));
    }
    else
    {
      puts("Failed to get DNS server IP");
    }

    setup_sntp();
  }
}

void wifi_init_sta(void)
{
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);

  esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASS,
          .threshold.authmode = WIFI_AUTH_WPA2_PSK,
      },
  };

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();

  // Get and print the Wi-Fi MAC address
  uint8_t mac[6];
  esp_err_t ret = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
  if (ret == ESP_OK)
  {
    printf("WiFi MAC address           : %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to get WiFi MAC address\n");
  }

  ESP_LOGD(TAG, "wifi_init_sta finished.");
}

void secondTask(void *parameter)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  printf("Second task running core   : %d\n", xPortGetCoreID());
  xSemaphoreGive(mutex);
  vTaskDelete(NULL);
}

void app_main()
{
  nvs_flash_init();
  mutex = xSemaphoreCreateMutex();

  const esp_app_desc_t *app_desc = esp_app_get_description();

  vTaskDelay(3000 / portTICK_PERIOD_MS); // wait for serial monitor

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

  vTaskDelay(50 / portTICK_PERIOD_MS);
  xSemaphoreTake(mutex, portMAX_DELAY);
  // -------------------------------------------------------------
  puts("");
  xSemaphoreGive(mutex);
  vTaskDelay(3000 / portTICK_PERIOD_MS);

  wifi_init_sta();

  ESP_LOGI(TAG, "*** APP_MAIN finished ***");
  fflush(stdout);
}
