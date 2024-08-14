#include "board_sdcard.h"

#include <dirent.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>


#ifdef HAS_SDCARD

static void configure_pin_iomux(uint8_t gpio_num)
{
  assert(gpio_num != (uint8_t)GPIO_NUM_NC);
  gpio_pulldown_dis(gpio_num);
}

void list_sdcard_root()
{
  const char mount_point[] = "/sdcard";
  configure_pin_iomux(SDMMC_SLOT_CONFIG_CLK);
  configure_pin_iomux(SDMMC_SLOT_CONFIG_CMD);
  configure_pin_iomux(SDMMC_SLOT_CONFIG_D0);

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  host.flags = SDMMC_HOST_FLAG;
  host.max_freq_khz = SDMMC_FREQ;
  host.slot = SDMMC_HOST_SLOT;

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  slot_config.clk = SDMMC_SLOT_CONFIG_CLK;
  slot_config.cmd = SDMMC_SLOT_CONFIG_CMD;
  slot_config.d0 = SDMMC_SLOT_CONFIG_D0;
  slot_config.d1 = SDMMC_SLOT_CONFIG_D1;
  slot_config.d2 = SDMMC_SLOT_CONFIG_D2;
  slot_config.d3 = SDMMC_SLOT_CONFIG_D3;

  slot_config.width = 1;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  sdmmc_card_t *card;

  // Initialize SD card
  esp_err_t ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (ret == ESP_OK)
  {
    printf("SD card mounted successfully\n");

    sdmmc_card_print_info(stdout, card);

    // List root directory
    DIR *dir = opendir(mount_point);
    if (dir != NULL)
    {
      struct dirent *ent;
      while ((ent = readdir(dir)) != NULL)
      {
        printf("File: %s\n", ent->d_name);
      }
      closedir(dir);
    }
    else
    {
      printf("Failed to open directory\n");
    }

    // Unmount SD card
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    printf("SD card unmounted\n");
  }
  else
  {
    printf("Failed to mount SD card\n");
  }
}
#endif

