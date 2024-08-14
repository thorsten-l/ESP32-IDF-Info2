#include <stdio.h>

#include <time.h>
#include <sys/time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_app_desc.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_sntp.h>

#include <config.h>
#include "app.h"
#include "board_led.h"
#include "board_sdcard.h"

static const char *TAG = "app";
SemaphoreHandle_t mutex;
bool setup_finished;

void delay(uint32_t ms)
{
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

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
    delay(1000);
  }

  now = time(NULL);

  // Check if time was set
  if (appTimeinfo.tm_year > (2023 - 1900))
  {
    appStartTime = now - (esp_log_timestamp() / 1000);
    localtime_r(&appStartTime, &appTimeinfo);
    printf("app start time             : %d-%02d-%02d %02d:%02d:%02d\n",
           appTimeinfo.tm_year + 1900, appTimeinfo.tm_mon + 1, appTimeinfo.tm_mday,
           appTimeinfo.tm_hour, appTimeinfo.tm_min, appTimeinfo.tm_sec);
  }
  else
  {
    puts("time not set");
  }

  localtime_r(&now, &appTimeinfo);
  printf("time                       : %d-%02d-%02d %02d:%02d:%02d\n",
         appTimeinfo.tm_year + 1900, appTimeinfo.tm_mon + 1, appTimeinfo.tm_mday,
         appTimeinfo.tm_hour, appTimeinfo.tm_min, appTimeinfo.tm_sec);
  printf("timezone                   : %s\n\n", getenv("TZ"));

  //-------------------------------------------------------------

  setup_finished = true;
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
    puts("\n--- retry to connect to the AP with SSID: " WIFI_SSID " ---");
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
