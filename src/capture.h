/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef CAPTURE_H
#define CAPTURE_H

#ifdef V4L2
#include <asm/types.h>
#include <linux/videodev2.h>
#endif

#ifdef VFW
#include <windows.h>
#include <vfw.h>
#endif

struct capture_info_t
{
#ifdef V4L2
  struct v4l2_capability vid_cap;
  struct v4l2_format vid_fmt;
  int video_fd;
  uint8_t *mmap_buf;
  int mmap_buf_len;
#endif
#ifdef VFW
  BITMAPINFO bmp;
  HWND hWnd;
  int jpeg_len;
  int callback_wait;
#endif
  uint8_t *buffer;
  int buffer_len;
  uint8_t *picture;
  int picture_len;
  int width,height;
  /* int convert_type; */
  int device_num; // FIXME - this should go away maybe
  int max_fps;
  int format;
  int channel;
};

int open_capture(struct capture_info_t *capture_info, char *dev_name);
int capture_image(struct capture_info_t *capture_info, int ID);
int close_capture(struct capture_info_t *capture_info);

#endif

