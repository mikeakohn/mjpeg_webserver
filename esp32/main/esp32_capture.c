
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_camera.h"

#include "esp32_capture.h"
#include "globals.h"
#include "user.h"

static camera_config_t camera_config =
{
  .pin_pwdn  = CAM_PIN_PWDN,
  .pin_reset = CAM_PIN_RESET,
  .pin_xclk = CAM_PIN_XCLK,
  .pin_sccb_sda = CAM_PIN_SIOD,
  .pin_sccb_scl = CAM_PIN_SIOC,

  .pin_d7 = CAM_PIN_D7,
  .pin_d6 = CAM_PIN_D6,
  .pin_d5 = CAM_PIN_D5,
  .pin_d4 = CAM_PIN_D4,
  .pin_d3 = CAM_PIN_D3,
  .pin_d2 = CAM_PIN_D2,
  .pin_d1 = CAM_PIN_D1,
  .pin_d0 = CAM_PIN_D0,
  .pin_vsync = CAM_PIN_VSYNC,
  .pin_href = CAM_PIN_HREF,
  .pin_pclk = CAM_PIN_PCLK,

  // These options create images around 228k (UXGA with quality of 12).

  // EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode.
  .xclk_freq_hz = 8000000,
  //.xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  // YUV422,GRAYSCALE,RGB565,JPEG
  .pixel_format = PIXFORMAT_JPEG,

  // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG.
  // The performance of the ESP32-S series has improved a lot, but JPEG
  // mode always gives better frame rates.
  .frame_size = FRAMESIZE_UXGA,
  //.frame_size = FRAMESIZE_QVGA,

   // 0-63, for OV series camera sensors, lower number means higher quality.
  .jpeg_quality = 12,
  // When jpeg mode is used, if fb_count more than one, the driver will
  // work in continuous mode.
  .fb_count = 1,
  //.fb_location = CAMERA_FB_IN_PSRAM,
  // CAMERA_GRAB_LATEST. Sets when buffers should be filled.
  .grab_mode = CAMERA_GRAB_WHEN_EMPTY
  //.grab_mode = CAMERA_GRAB_LATEST
};

int open_capture(CaptureInfo *capture_info, char *dev_name)
{
  gpio_config_t io_conf = { };

  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << CAM_PIN_28V);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;

  // Power up the camera if PWDN pin is defined.
#if 0
  if (CAM_PIN_PWDN != -1)
  {
    io_conf.pin_bit_mask |= (1ULL << CAM_PIN_PWDN);
  }
#endif

  gpio_config(&io_conf);

  gpio_set_level(CAM_PIN_28V, 0);
  //gpio_set_level(CAM_PIN_PWDN, 0);

  // Initialize the camera.
  esp_err_t err = esp_camera_init(&camera_config);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Camera Init Failed");
    return -1;
  }

  return 0;
}

static size_t jpg_encode_stream(
  void *arg,
  size_t index,
  const void *data,
  size_t len)
{
  User *user = (User *)arg;

  if (user->jpeg_len == 0)
  {
    user->jpeg = (uint8_t *)malloc(JPEG_MAX_SIZE);
    user->jpeg_len = JPEG_MAX_SIZE;
  }

  if (len > user->jpeg_len)
  {
    ESP_LOGE(TAG, "JPEG len %d is bigger than buffer len %d\n",
      len,
      user->jpeg_len);

    user->content_length = 0;

    return 0;
  }

  memcpy(user->jpeg, data, len);
  user->content_length = len;

  return len;
}

int capture_image(CaptureInfo *capture_info, int id)
{
  uint32_t fb_len = 0;

  User *user = users[id];

  if (capture_info->fb != NULL)
  {
    esp_camera_fb_return(capture_info->fb);
  }

  camera_fb_t *fb = esp_camera_fb_get();

  capture_info->fb = fb;

  if (fb == NULL)
  {
    ESP_LOGE(TAG, "Camera capture failed");
    return -2;
  }

  esp_err_t res = ESP_OK;

  if (fb->format == PIXFORMAT_JPEG)
  {
    //ESP_LOGI(TAG, "PIXFORMAT_JPEG");

    fb_len = fb->len;
    user->jpeg = fb->buf;
    user->content_length = fb->len;
  }
    else
  {
    //ESP_LOGI(TAG, "compress JPEG");

    res = frame2jpg_cb(fb, 80, jpg_encode_stream, user) ? ESP_OK : ESP_FAIL;

    fb_len = user->content_length;
  }

  //esp_camera_fb_return(fb);

  ESP_LOGI(TAG, "JPG: %luKB  content_length: %d", (uint32_t)(fb_len / 1024), user->content_length);

  return res == ESP_OK ? user->content_length : -1;
}

int close_capture(CaptureInfo *capture_info)
{
  return 0;
}

