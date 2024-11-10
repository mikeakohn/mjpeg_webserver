/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
// #include <winuser.h>
#include <vfw.h>

#include "capture.h"
#include "globals.h"
#include "jpeg_compress.h"

#if 0
#define WM_CAP_START WM_USER
#define WM_CAP_SET_CALLBACK_FRAME WM_CAP_START + 5
#define WM_CAP_DRIVER_CONNECT WM_CAP_START + 10
#define WM_CAP_DRIVER_DISCONNECT WM_CAP_START + 11
#define WM_CAP_SET_VIDEOFORMAT WM_CAP_START + 45
#define WM_CAP_GRAB_FRAME WM_CAP_START + 60
#define WM_CAP_STOP WM_CAP_START + 68
#endif

#define MAX_CAPTURE_DEVICES 128

#if 0
static struct capture_info_ptr_t
{
  CaptureInfo *capture_info;
} capture_info_ptr[MAX_CAPTURE_DEVICES];
#endif

static CaptureInfo *capture_info_ptr[MAX_CAPTURE_DEVICES];

static int vfw_count = 0;

#if 0
typedef struct
{
  uint8_t *lpData;
  DWORD dwBufferLength;
} *LPVIDEOHDR;
#endif

LRESULT CALLBACK capVideoStreamCallback(HWND hWnd, LPVIDEOHDR lpVHdr)
{
  int i;

#ifdef DEBUG
printf("In callback\n");
fflush(stdout);
#endif

  for (i = 0; i < vfw_count; i++)
  {
    if (capture_info_ptr[i] == NULL) { return -1; }
    if (capture_info_ptr[i]->hWnd) { break; }
  }

  if (i == vfw_count) { return -2; }

#ifdef DEBUG
printf("In callback: Copy\n");
fflush(stdout);
#endif

  memcpy(capture_info_ptr[i]->buffer, lpVHdr->lpData, lpVHdr->dwBufferLength);
  capture_info_ptr[i]->callback_wait = 0;
  capture_info_ptr[i]->jpeg_len = lpVHdr->dwBufferLength;

#ifdef DEBUG
printf("exit callback\n");
fflush(stdout);
#endif

  return 0;
}

int open_capture(CaptureInfo *capture_info, char *dev_name)
{
  int i;

  if (vfw_count == 0)
  {
    for (i = 0; i < MAX_CAPTURE_DEVICES; i++) { capture_info_ptr[i] = NULL; }
  }

  // capture_info_ptr[vfw_count++] = (CaptureInfo *)malloc(sizeof(CaptureInfo *));
  capture_info_ptr[vfw_count++] = capture_info;

#ifdef DEBUG
printf("width=%d height=%d\n", capture_info->width, capture_info->height);
#endif

  capture_info->hWnd = capCreateCaptureWindow(
    "mjpeg_webserver",
    0,
    capture_info->width,
    capture_info->height,
    capture_info->width,
    capture_info->height,
    NULL,
    0);

#ifdef DEBUG
printf("capture_info->hWnd=%d\n", capture_info->hWnd);
#endif

  if (capture_info->hWnd == 0) { return -1; }

  if (!capDriverConnect(capture_info->hWnd, atoi(dev_name)))
  {
#ifdef DEBUG
printf("capDriverConnect failed: capture_info->hWnd=%d\n", capture_info->hWnd);
#endif
    return -2;
  }

  memset(&capture_info->bmp, 0, sizeof(BITMAPINFO));
  capture_info->bmp.bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
  capture_info->bmp.bmiHeader.biBitCount  = 24;
  capture_info->bmp.bmiHeader.biPlanes    = 1;
  capture_info->bmp.bmiHeader.biWidth     = capture_info->width;
  capture_info->bmp.bmiHeader.biHeight    = capture_info->height;
  capture_info->bmp.bmiHeader.biSizeImage =
    capture_info->width * capture_info->height * 3;
  capture_info->bmp.bmiHeader.biCompression = BI_RGB;

  if (!capSetVideoFormat(capture_info->hWnd, &capture_info->bmp, sizeof(BITMAPINFO)))
  {
#ifdef DEBUG
printf("capSetVideoFormat failed: capture_info->hWnd=%d\n", capture_info->hWnd);
#endif
    return -3;
  }

#ifdef DEBUG
printf("capSetVideoFormat passed: width=%d height=%d\n",
  capture_info->bmp.bmiHeader.biWidth,
  capture_info->bmp.bmiHeader.biHeight);
#endif

  capture_info->buffer_len = capture_info->width * capture_info->height * 3;
  capture_info->buffer = (uint8_t *)malloc(capture_info->buffer_len);

  //if (!SendMessage(capture_info->hWnd,WM_CAP_SET_CALLBACK_FRAME,0,(LPARAM)capVideoStreamCallback))
  if (!capSetCallbackOnFrame(capture_info->hWnd, capVideoStreamCallback))
  {
#ifdef DEBUG
printf("capSetCallbackFrame failed: capture_info->hWnd=%d\n", capture_info->hWnd);
#endif
    return -4;
  }

  return 0;
}

int capture_image(CaptureInfo *capture_info, int id)
{
  int count;

  if (users[id]->jpeg_len == 0)
  {
#ifdef DEBUG
printf("growing jpeg buffer\n");
fflush(stdout);
#endif
// FIXME this should be dynamic instead of wasting memory.. easy to fix
//       also need a way to deallocate this crap

    users[id]->jpeg_len = 128000;
    users[id]->jpeg = (uint8_t *)malloc(users[id]->jpeg_len);
  }

  capture_info->callback_wait=1;

#ifdef DEBUG
printf("capture_image: enter %d  %x %s:%d\n",
  (int)capture_info->hWnd,
  WM_CAP_GRAB_FRAME,
  __FILE__,
  __LINE__);
fflush(stdout);
#endif

  if (!capGrabFrame(capture_info->hWnd))
  {
#ifdef DEBUG
printf("capture_image: grab error\n");
fflush(stdout);
#endif
    memset(capture_info->buffer, 0, capture_info->buffer_len);
    capture_info->callback_wait = 0;
  }

#ifdef DEBUG
printf("capture_image: waiting\n");
fflush(stdout);
#endif

  count = 0;
  while (capture_info->callback_wait == 1)
  {
    Sleep(1);
#ifdef DEBUG
printf("Waiting %d\n", count);
fflush(stdout);
#endif
    if (count == 100) { break; }
    count++;
  }
#ifdef DEBUG
printf("capture_image: exit %d %p %d %p %d  %d %d %d\n",
  count,
  capture_info->buffer,
  video[users[id]->video_num].capture_info->buffer_len,
  users[id]->jpeg,
  users[id]->jpeg_len,
  video[users[id]->video_num].capture_info->width,
  video[users[id]->video_num].capture_info->height,
  users[id]->jpeg_quality);
fflush(stdout);
#endif

  //return capture_info->buffer;

  return jpeg_compress(
    capture_info->buffer,
    video[users[id]->video_num].capture_info->buffer_len,
    users[id]->jpeg,
    users[id]->jpeg_len,
    video[users[id]->video_num].capture_info->width,
    video[users[id]->video_num].capture_info->height,
    3,
    users[id]->jpeg_quality);
}

int close_capture(CaptureInfo *capture_info)
{
  SendMessage(capture_info->hWnd,WM_CAP_SET_CALLBACK_FRAME, 0, 0);
  SendMessage(capture_info->hWnd,WM_CAP_STOP, 0, 0);

  free(capture_info->buffer);
  if (capture_info->picture != 0) { free(capture_info->picture); }

  return 0;
}

