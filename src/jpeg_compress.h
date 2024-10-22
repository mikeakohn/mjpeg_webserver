/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef JPEG_COMPRESS
#define JPEG_COMPRESS

int jpeg_decompress(
  uint8_t *jpeg_buffer,
  int jpeg_buffer_len,
  uint8_t *uncomp_buffer,
  int uncomp_buffer_len,
  int *width,
  int *height,
  int bytes_per_pixel);

int jpeg_compress(
  uint8_t *uncomp_buffer,
  int uncomp_buffer_len,
  uint8_t *jpeg_buffer,
  int jpeg_buffer_len,
  int width,
  int height,
  int bytes_per_pixel,
  int quality);

#endif

