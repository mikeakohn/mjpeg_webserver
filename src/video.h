/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

typedef struct Video
{
  char *filename;
  long *index;
  int fps;
  int total_frames;
  struct timeval tv_start;
  struct timeval tv_total;
#ifdef ENABLE_CAPTURE
  struct capture_info_t *capture_info;
#endif
#ifdef WITH_MMAP
  long file_len;
  uint8_t *mem;
#ifndef WINDOWS
  int fd;
#else
  HANDLE fd;
  HANDLE mem_handle;
#endif
#endif
} Video;

#endif

