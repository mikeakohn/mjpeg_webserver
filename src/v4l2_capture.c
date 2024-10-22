/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "capture.h"
#include "globals.h"
#include "jpeg_compress.h"

void bgr2rgb(struct capture_info_t *capture_info)
{
  int t, c;

  for (t = 0; t < capture_info->buffer_len; t = t + 3)
  {
    c=capture_info->buffer[t + 2];
    capture_info->buffer[t + 2] = capture_info->buffer[t];
    capture_info->buffer[t] = c;
  }
}

uint8_t *convert_yuyv(struct capture_info_t *capture_info)
{
  int len, ptr;
  int u, v, u1, uv1, v1, y1;
  int r, g, b;
  int x;


  len = (capture_info->width * capture_info->height) << 1; /* w*h*2 */
  ptr = 0;

  for (x = 0; x < len; x = x + 4)
  {
    u = capture_info->buffer[x + 1] - 128;
    v = capture_info->buffer[x + 3] - 128;

    v1 = (5727 * v);
    uv1 = -(1617 * u) - (2378 * v);
    u1 = (8324 * u);

    y1 = capture_info->buffer[x] << 12;
    r = (y1 + v1) >> 12;
    g = (y1 + uv1) >> 12;
    b = (y1 + u1) >> 12;

    if (r > 255) { r = 255; }
    if (g > 255) { g = 255; }
    if (b > 255) { b = 255; }

    if (r < 0) { r = 0; }
    if (g < 0) { g = 0; }
    if (b < 0) { b = 0; }

    capture_info->picture[ptr + 0] = r;
    capture_info->picture[ptr + 1] = g;
    capture_info->picture[ptr + 2] = b;

    y1=capture_info->buffer[x + 2] << 12;
    r=(y1 + v1) >> 12;
    g=(y1 + uv1) >> 12;
    b=(y1 + u1) >> 12;

    if (r > 255) { r = 255; }
    if (g > 255) { g = 255; }
    if (b > 255) { b = 255; }

    if (r < 0) { r = 0; }
    if (g < 0) { g = 0; }
    if (b < 0) { b = 0; }

    capture_info->picture[ptr + 3] = r;
    capture_info->picture[ptr + 4] = g;
    capture_info->picture[ptr + 5] = b;

    ptr = ptr + 6;
  }

  return capture_info->picture;
}

uint8_t *convert_bayer(struct capture_info_t *capture_info)
{
  int x, y, c;
  int ptr;

  ptr = 0;

  for (y = 0; y < capture_info->height; y++)
  {
    c = (y * (capture_info->width));

    for (x = 0; x < capture_info->width; x = x + 2)
    {
      if ((y % 2) == 0)
      {
        capture_info->picture[ptr++] = capture_info->buffer[c + capture_info->width + 1];
        capture_info->picture[ptr++] = capture_info->buffer[c + capture_info->width];
        capture_info->picture[ptr++] = capture_info->buffer[c];

        capture_info->picture[ptr++] = capture_info->buffer[c + capture_info->width + 1];
        capture_info->picture[ptr++] = capture_info->buffer[c + 1];
        capture_info->picture[ptr++] = capture_info->buffer[c];
      }
        else
      {
        capture_info->picture[ptr++] = capture_info->buffer[c + 1];
        capture_info->picture[ptr++] = capture_info->buffer[c];
        capture_info->picture[ptr++] = capture_info->buffer[c - capture_info->width];

        capture_info->picture[ptr++] = capture_info->buffer[c + 1];
        capture_info->picture[ptr++] = capture_info->buffer[c - capture_info->width + 1];
        capture_info->picture[ptr++] = capture_info->buffer[c - capture_info->width];
      }

      c = c + 2;
    }
  }

  return capture_info->picture;
}

int open_capture(struct capture_info_t *capture_info, char *dev_name)
{
  // char dev_name[]={"/dev/video0"};
  // char dev_name[128];
  char *fourcc;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer buf;
  int c;

  // sprintf(dev_name,"/dev/video%d",capture_info->device_num);

  printf("Video Device: %s\n", dev_name);

  // video_fd = open(dev_name, O_RDONLY, 0);
  // video_fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

  capture_info->video_fd = open(dev_name, O_RDWR, 0);

  if (capture_info->video_fd == -1)
  {
    printf("Problem opening %d\n",capture_info->video_fd);
    return -1;
  }

  if (ioctl(capture_info->video_fd, VIDIOC_QUERYCAP, &capture_info->vid_cap) == -1)
  {
    printf("ioctl error: VIDIOC_QUERYCAP  errno=%d\n", errno);
    return -1;
  }

#ifdef DEBUG
  printf("      driver: %16s\n",   capture_info->vid_cap.driver);
  printf("        card: %32s\n",   capture_info->vid_cap.card);
  printf("    bus_info: %32s\n",   capture_info->vid_cap.bus_info);
  printf("     version: %d\n",     capture_info->vid_cap.version);
  printf("capabilities: 0x%08x\n", capture_info->vid_cap.capabilities);
  printf("\n");
#endif

  if ((capture_info->vid_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
  {
    close(capture_info->video_fd);
    printf("This device doesn't support frame capture\n");
    return -3;
  }

  // c=capture_info->channel;
  if (capture_info->channel != -1)
  {
    ioctl(capture_info->video_fd, VIDIOC_S_INPUT, &capture_info->channel);
  }

  // c=V4L2_STD_NTSC_M;
  if (capture_info->format!=0)
  {
    ioctl(capture_info->video_fd, VIDIOC_S_STD, &capture_info->format);
  }

  memset(&capture_info->vid_fmt, 0, sizeof(capture_info->vid_fmt));

#if 0
  if (ioctl(capture_info->video_fd, VIDIOC_G_FMT, &capture_info->vid_fmt)==-1)
  {
    printf("ioctl error: VIDIOC_G_FMT\n");
    return -7;
  }
#endif

  capture_info->vid_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  capture_info->vid_fmt.fmt.pix.width = capture_info->width;
  capture_info->vid_fmt.fmt.pix.height = capture_info->height;
  // capture_info->vid_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
  capture_info->vid_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
  // capture_info->vid_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
  // capture_info->vid_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
  capture_info->vid_fmt.fmt.pix.field = V4L2_FIELD_ANY;

  fourcc = (char *)&capture_info->vid_fmt.fmt.pix.pixelformat;
#ifdef DEBUG
  printf(" pixelformat: %08x %c%c%c%c\n"
    capture_info->vid_fmt.fmt.pix.pixelformat,
    fourcc[0],
    fourcc[1],
    fourcc[2],
    fourcc[3]);
#endif

    capture_info->vid_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    // capture_info->vid_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;

  //if (ioctl(capture_info->video_fd, VIDIOC_TRY_FMT, &capture_info->vid_fmt) == -1)
  if (ioctl(capture_info->video_fd, VIDIOC_S_FMT, &capture_info->vid_fmt) == -1)
  {
    close(capture_info->video_fd);
    printf("ioctl error: VIDIOC_S_FMT errno=%d\n", errno);
    return -1;
  }

#ifdef DEBUG
  printf(" pixelformat: %08x %c%c%c%c\n",
    capture_info->vid_fmt.fmt.pix.pixelformat,
    fourcc[0],
    fourcc[1],
    fourcc[2],
    fourcc[3]);
  printf(" byteperline: %d\n",   capture_info->vid_fmt.fmt.pix.bytesperline);
  printf("   sizeimage: %d\n",   capture_info->vid_fmt.fmt.pix.sizeimage);
  printf("  colorspace: %08x\n", capture_info->vid_fmt.fmt.pix.colorspace);
#endif

  if ((capture_info->vid_cap.capabilities & V4L2_CAP_READWRITE) == 0)
  {
    memset(&req, 0, sizeof(req));
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    req.count = 1;

    if (ioctl(capture_info->video_fd, VIDIOC_REQBUFS, &req) == -1)
    {
      close(capture_info->video_fd);
      printf("ioctl error: VIDIOC_REQBUFS errno=%d\n", errno);
      return -1;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(capture_info->video_fd, VIDIOC_QUERYBUF, &buf) == -1)
    {
      close(capture_info->video_fd);
      printf("ioctl error: VIDIOC_QUERYBUF errno=%d\n", errno);
      return -9;
    }

    capture_info->mmap_buf_len = buf.length;
    capture_info->mmap_buf =
      mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, capture_info->video_fd, buf.m.offset);

    if (capture_info->mmap_buf == (void *)-1)
    {
      close(capture_info->video_fd);
      printf("mmap error: errno=%d\n", errno);
      return -10;
    }

    c = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(capture_info->video_fd, VIDIOC_STREAMON, &c) == -1)
    {
      close(capture_info->video_fd);
      printf("ioctl error: VIDIOC_STREAMON errno=%d\n", errno);
      return -9;
    }
  }

  if (capture_info->vid_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR8 ||
      capture_info->vid_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
  {
    capture_info->picture =
      (uint8_t *)malloc(capture_info->width*capture_info->height * 3);
  }

  capture_info->buffer_len = capture_info->vid_fmt.fmt.pix.sizeimage;
  capture_info->buffer = (uint8_t *)malloc(capture_info->buffer_len);

  return 0;
}

int read_frame(struct capture_info_t *capture_info, uint8_t *buffer, int len)
{
  struct v4l2_buffer buf;
  int c, n;

  c = 0;
  n = 0;

  if ((capture_info->vid_cap.capabilities&V4L2_CAP_READWRITE) != 0)
  {
    while (c<len)
    {
      n = read(capture_info->video_fd, buffer+c, len - c);

      if (n < 0)
      {
#ifdef DEBUG
printf("v4l2 read()=%d error.  errno=%d  buffer=%d/%d\n", n, errno, c, len);
#endif
        break;
      }
      c = c + n;
    }
  }
    else
  {
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(capture_info->video_fd, VIDIOC_QBUF, &buf) == -1)
    {
      printf("ioctl error: VIDIOC_QBUF errno=%d\n", errno);
      //return -9;
    }

    if (ioctl(capture_info->video_fd, VIDIOC_DQBUF, &buf) == -1)
    {
      printf("ioctl error: VIDIOC_DQBUF errno=%d\n", errno);
      //return -9;
    }

    memcpy(buffer, capture_info->mmap_buf, buf.length);
  }

  return c;
}

int capture_image(struct capture_info_t *capture_info, int id)
{
  uint8_t *cap_buffer = 0;

  if (users[id]->jpeg_len == 0)
  {
// FIXME this should be dynamic instead of wasting memory.. easy to fix
//       also need a way to deallocate this crap

    // users[id]->jpeg_len = 20480;
    users[id]->jpeg_len = 128000;
    users[id]->jpeg = (uint8_t *)malloc(users[id]->jpeg_len);
  }

  if (capture_info->vid_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
  {
    return read_frame(capture_info, users[id]->jpeg, capture_info->buffer_len);
  }

  read_frame(capture_info, capture_info->buffer, capture_info->buffer_len);
  cap_buffer = capture_info->buffer;

  if (capture_info->vid_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR24)
  {
    bgr2rgb(capture_info);
  }
    else
  if (capture_info->vid_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR8)
  {
    cap_buffer=convert_bayer(capture_info);
  }
    else
  if (capture_info->vid_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
  {
    cap_buffer=convert_yuyv(capture_info);
  }


  return jpeg_compress(
    cap_buffer,
    video[users[id]->video_num].capture_info->buffer_len,
    users[id]->jpeg,
    users[id]->jpeg_len,
    video[users[id]->video_num].capture_info->width,
    video[users[id]->video_num].capture_info->height,
    3,
    users[id]->jpeg_quality);
}

int close_capture(struct capture_info_t *capture_info)
{
  free(capture_info->buffer);

  if (capture_info->picture != NULL)
  {
    free(capture_info->picture);
  }

  close(capture_info->video_fd);

  return 0;
}

