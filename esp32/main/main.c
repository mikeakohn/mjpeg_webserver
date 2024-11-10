
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "sdmmc.h"
#include "wifi.h"

#include "config.h"
#include "server.h"

static const char *TAG = "www";

void app_main(void)
{
  ESP_LOGI(TAG, "Starting up!");

  sdmmc_card_t *sdcard = sdcard_mount();

#if 0
  if (sdcard != NULL)
  {
    FILE *fp = fopen("/sdcard/htdocs/test.txt", "r");
    if (fp != NULL)
    {
      char buffer[128];
      memset(buffer, 0, sizeof(buffer));
      fread(buffer, 1, sizeof(buffer), fp);
      ESP_LOGI(TAG, "%s\n", buffer);
      fclose(fp);
    }
      else
    {
      ESP_LOGI(TAG, "couldn't open file.");
    }
  }
#endif

  char *args[] = { "", "-d", "-f", "/sdcard/httpd_cl.cnf" };
  const int count = sizeof(args) / sizeof(char *);

  Config config;

  ESP_LOGI(TAG, "Config init.");
  config_init(&config, count, args);

  ESP_LOGI(TAG, "Config read in.");

  config_dump(&config);
  ESP_LOGI(TAG, "Config dump.");

  if (config.wifi_is_ap)
  {
    wifi_start_ap(config.wifi_ssid, config.wifi_password);
  }
    else
  {
    wifi_start_client(config.wifi_ssid, config.wifi_password);
  }

  server_run(&config);
  config_destroy(&config);

#if 0
  while (1)
  {
    sleep(1);
    //blink_led();

    // Toggle the LED state.

    //s_led_state = !s_led_state;
    //vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
  }
#endif

  sdcard_unmount(sdcard);
}

