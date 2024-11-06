/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
//#include "sd_test_io.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#include "sdmmc.h"

static const char *TAG = "sdmmc";

#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS

const char *names[] =  { "CLK", "CMD", "D0", "D1", "D2", "D3"};

const int pins[] =
{
  CONFIG_EXAMPLE_PIN_CLK,
  CONFIG_EXAMPLE_PIN_CMD,
  CONFIG_EXAMPLE_PIN_D0,
  //#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
  CONFIG_EXAMPLE_PIN_D1,
  CONFIG_EXAMPLE_PIN_D2,
  CONFIG_EXAMPLE_PIN_D3
  //#endif
};

const int pin_count = sizeof(pins) / sizeof(pins[0]);

pin_configuration_t config =
{
  .names = names,
  .pins = pins,
};

#endif

sdmmc_card_t *sdcard_mount()
{
  esp_err_t ret;

  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned
  // and formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config =
  {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  sdmmc_card_t *sdcard;

  ESP_LOGI(TAG, "Initializing SD card");

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
  // Please check its source code and implement error recovery when developing
  // production applications.

  ESP_LOGI(TAG, "Using SDMMC peripheral");

  // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
  // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
  // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // For SoCs where the SD power can be supplied both via an internal or
  // external (e.g. on-board LDO) power supply. When using specific IO pins
  // (which can be used for ultra high-speed SDMMC) to connect to the SD card
  // and the internal LDO power supply, we need to initialize the power
  // supply first.
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
  sd_pwr_ctrl_ldo_config_t ldo_config =
  {
    .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
  };

  sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

  ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
    return;
  }

    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

  // This initializes the slot without card detect (CD) and write protect (WP)
  // signals. Modify slot_config.gpio_cd and slot_config.gpio_wp if your board
  // has these signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  // Set bus width to use:
//#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
  slot_config.width = 4;
//#else
//  slot_config.width = 1;
//#endif

  // On chips where the GPIOs used for SD card can be configured, set them in
  // the slot_config structure:
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
  slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
  slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
  slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;
//#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
  slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
  slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
  slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
//#endif
#endif

  // Enable internal pullups on enabled pins. The internal pullups
  // are insufficient however, please make sure 10k external pullups are
  // connected on the bus. This is for debug / example purpose only.
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "Mounting filesystem");
  ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &sdcard);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG,
        "Failed to mount filesystem. "
        "If you want the card to be formatted, set the "
        "EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
      else
    {
      ESP_LOGE(TAG,
        "Failed to initialize the card (%s). "
        "Make sure SD card lines have pull-up resistors in place.",
        esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
        check_sd_card_pins(&config, pin_count);
#endif
    }

    return NULL;
  }

  ESP_LOGI(TAG, "Filesystem mounted");

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, sdcard);

  return sdcard;
}

esp_err_t sdcard_unmount(sdmmc_card_t *sdcard)
{
  // All done, unmount partition and disable SDMMC peripheral
  esp_vfs_fat_sdcard_unmount(MOUNT_POINT, sdcard);
  ESP_LOGI(TAG, "Card unmounted");

  // Deinitialize the power control driver if it was used
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
  ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to delete the on-chip LDO power control driver");
    return ret;
  }
#endif

  return ESP_OK;
}

