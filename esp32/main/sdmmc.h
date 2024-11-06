
#ifndef SDMMC_H
#define SDMMC_H

#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"

#define CONFIG_EXAMPLE_PIN_CLK 14
#define CONFIG_EXAMPLE_PIN_CMD 15
#define CONFIG_EXAMPLE_PIN_D0  2
#define CONFIG_EXAMPLE_PIN_D1  4
#define CONFIG_EXAMPLE_PIN_D2  12
#define CONFIG_EXAMPLE_PIN_D3  13

sdmmc_card_t *sdcard_mount();
esp_err_t sdcard_unmount(sdmmc_card_t *sdcard;);

#endif

