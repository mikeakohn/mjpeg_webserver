
#ifndef ESP_CAPTURE
#define ESP_CAPTURE

#include "capture.h"

#define JPEG_MAX_SIZE (96 * 1024)
#define TAG "camera"

#define CAM_PIN_28V     32 // ESP32-Cam needs this pin high to turn on camera.
#define CAM_PIN_PWDN    -1 // Power down is not used.
#define CAM_PIN_RESET   -1 // Software reset will be performed.
#define CAM_PIN_SIOC    27
#define CAM_PIN_SIOD    26
#define CAM_PIN_XCLK    0
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

#define CAM_PIN_D0       5
#define CAM_PIN_D1      18 
#define CAM_PIN_D2      19
#define CAM_PIN_D3      21
#define CAM_PIN_D4      36
#define CAM_PIN_D5      39
#define CAM_PIN_D6      34
#define CAM_PIN_D7      35

/*
#define CAM_PIN_PWDN    -1 // Power down is not used.
#define CAM_PIN_RESET   -1 // Software reset will be performed.
#define CAM_PIN_SIOC    27
#define CAM_PIN_SIOD    26
#define CAM_PIN_XCLK    21
#define CAM_PIN_HREF    23
#define CAM_PIN_VSYNC   25
#define CAM_PIN_PCLK    22

#define CAM_PIN_D0       4
#define CAM_PIN_D1       5
#define CAM_PIN_D2      18
#define CAM_PIN_D3      19
#define CAM_PIN_D4      36
#define CAM_PIN_D5      39
#define CAM_PIN_D6      34
#define CAM_PIN_D7      35
*/

#endif

