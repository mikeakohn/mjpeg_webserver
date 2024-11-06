
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"

static void wifi_init_nvs_flash()
{
  // Setting up NVS (Non Volatile Storage) appears to be needed for WiFi.
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);
}

void wifi_event_handler_ap(
  void *arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void *event_data)
{
  if (event_base == WIFI_EVENT)
  {
    switch (event_id)
    {
      case WIFI_EVENT_AP_START:
      {
        esp_wifi_connect();
        ESP_LOGI("wifi", "WIFI_EVENT_AP_START");

        break;
      }
      case WIFI_EVENT_AP_STACONNECTED:
      {
        wifi_event_ap_staconnected_t *event =
          (wifi_event_ap_staconnected_t *)event_data;

        ESP_LOGI("wifi", "station " MACSTR " join, AID=%d",
          MAC2STR(event->mac),
          event->aid);

        esp_wifi_connect();

        break;
      }
      case WIFI_EVENT_AP_STADISCONNECTED:
      {
        wifi_event_ap_stadisconnected_t *event =
          (wifi_event_ap_stadisconnected_t *)event_data;

        ESP_LOGI("wifi", "station " MACSTR " leave, AID=%d",
          MAC2STR(event->mac),
          event->aid);

        break;
      }
      case WIFI_EVENT_HOME_CHANNEL_CHANGE:
      {
        wifi_event_ap_stadisconnected_t *event =
          (wifi_event_ap_stadisconnected_t *)event_data;

        ESP_LOGI("wifi", "station " MACSTR " home channel change, AID=%d",
          MAC2STR(event->mac),
          event->aid);

        break;
      }
      default:
      {
        ESP_LOGI("wifi", "Unknown AP event %ld 0x%08lx", event_id, event_id);

        break;
      }
    }
  }
    else
  if (event_base == IP_EVENT)
  {
    switch (event_id)
    {
      case IP_EVENT_STA_GOT_IP:
      {
        ESP_LOGI("wifi", "IP_EVENT_STA_GOT_IP");

        break;
      }
      default:
      {
        ESP_LOGI("wifi", "Unknown IP event %ld 0x%08lx", event_id, event_id);

        break;
      }
    }
  } 
}

void wifi_event_handler_client(
  void *arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void *event_data)
{
  switch (event_id)
  {
    case WIFI_EVENT_STA_START:
    {
      ESP_LOGI("wifi", "esp_wifi_connect();");
      esp_wifi_connect();

      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED:
    {
      esp_wifi_connect();
      ESP_LOGI("wifi", "Disconnected.. retry to connect to the AP");

      break;
    }
    case IP_EVENT_STA_GOT_IP:
    {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI("wifi", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

      break;
    }
  }
}

int wifi_start_ap(const char *ssid, const char *password)
{
  wifi_init_nvs_flash();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_ip_info_t ip_info;
  IP4_ADDR(&ip_info.ip, 192,168,2,1);
  IP4_ADDR(&ip_info.gw, 192,168,2,1);
  IP4_ADDR(&ip_info.netmask, 255,255,255,0);

  esp_netif_t *wifi_ap = esp_netif_create_default_wifi_ap();

  esp_netif_dhcps_stop(wifi_ap);
  esp_netif_set_ip_info(wifi_ap, &ip_info);
  esp_netif_dhcps_start(wifi_ap);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    &wifi_event_handler_ap,
    NULL,
    NULL));

  wifi_config_t wifi_config = { };

  snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", ssid);
  wifi_config.ap.ssid_len = strlen(ssid);
  wifi_config.ap.channel = CHANNEL;
  snprintf((char *)wifi_config.ap.password, sizeof(wifi_config.ap.password), "%s", password);
  wifi_config.ap.max_connection = 8;
//  This WPA3 isn't working on this ESP32-Cam board.
//#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
//  wifi_config.ap.authmode = WIFI_AUTH_WPA3_PSK;
//  wifi_config.ap.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
//#else
  wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
//#endif
  wifi_config.ap.pmf_cfg.required = true;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(
    "wifi",
    "wifi_init_softap finished. SSID:%s password:%s channel:%d",
    ssid, password, CHANNEL);

  return 0;
}

int wifi_start_client(const char *ssid, const char *password)
{
  wifi_init_nvs_flash();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    &wifi_event_handler_client,
    NULL,
    NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    IP_EVENT,
    IP_EVENT_STA_GOT_IP,
    &wifi_event_handler_client,
    NULL,
    NULL));

  wifi_config_t wifi_config = { };

  snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", ssid);
  snprintf((char *)wifi_config.ap.password, sizeof(wifi_config.ap.password), "%s", password);

  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(
    "wifi",
    "wifi_init finished. SSID:%s password:%s channel:%d",
    ssid, password, CHANNEL);

  return 0;
}

