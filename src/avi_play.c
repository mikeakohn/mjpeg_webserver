/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "avi_play.h"
#include "functions.h"
#include "globals.h"
#include "http_headers.h"
#include "mime_types.h"
#include "avi_parse.h"
#include "user.h"

int avi_init(const char *filename)
{
  FILE *in = fopen(filename, "rb");

  if (in == 0)
  {
    printf("Cannot open AVI file: %s\n", filename);
    return -1;
  }

  printf("Indexing AVI file %d: '%s'\n", video_count, filename);

  memset(&video[video_count], 0, sizeof(Video));

  video[video_count].filename = malloc(strlen(filename) + 1);

  strcpy(video[video_count].filename, filename);
  parse_riff(in, &video[video_count]);

  /* gettimeofday(&video[video_count].tv_start, DST_NONE); */
  gettimeofday(&video[video_count].tv_start, 0);

  video[video_count].tv_total.tv_sec =
    video[video_count].total_frames / video[video_count].fps;

  video[video_count].tv_total.tv_usec =
    (int)((((double)video[video_count].total_frames /
            (double)video[video_count].fps) -
            (double)video[video_count].tv_total.tv_sec) * (double)1000000);

#ifdef DEBUG
  if (debug == 1)
  {
    printf("fps=%d total_frames=%d sec=%d usec=%d\n",
      video[video_count].fps,
      video[video_count].total_frames,
      (int)video[video_count].tv_total.tv_sec,
      (int)video[video_count].tv_total.tv_usec);
  }
#endif

#ifdef WITH_MMAP
  fseek(in,0,SEEK_END);
  video[video_count].file_len = ftell(in);
#ifndef WINDOWS
  video[video_count].fd = open(video[video_count].filename, O_RDONLY);

  video[video_count].mem = (uint8_t *)mmap(
    NULL,
    video[video_count].file_len,
    PROT_READ,
    MAP_SHARED,
    video[video_count].fd,
    0);
#else
  video[video_count].fd = CreateFile(
    video[video_count].filename,
    FILE_READ_DATA,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_READONLY,
    NULL);

  // Security.nLength=sizeof(Security);
  // Security.lpSecurityDescriptor = NULL;

  video[video_count].mem_handle = CreateFileMapping(
    video[video_count].fd,
    NULL,
    PAGE_READONLY,
    0,
    video[video_count].file_len,
    NULL);

  video[video_count].mem = (uint8_t *)MapViewOfFile(
    video[video_count].mem_handle,
    FILE_MAP_READ,
    0,
    0,
    video[video_count].file_len);
#endif
#endif

  video_count++;
  fclose(in);

  return 0;
}

int avi_play_calc_frame(User *user)
{
  uint8_t temp[8];
  struct timeval tv_now;
  int frame;
  int t, r;
#ifndef WITH_MMAP
// int l;
#endif

  gettimeofday(&tv_now, 0);

  t =
    ((tv_now.tv_sec  - video[user->video_num].tv_start.tv_sec) * 1000) +
    ((tv_now.tv_usec - video[user->video_num].tv_start.tv_usec) / 1000);

  r = (int)(((double)t / 1000) * (double)video[user->video_num].fps);
  t = r / video[user->video_num].total_frames;
  r = r % video[user->video_num].total_frames;

  if (t > 1000 || r < 0)
  {
    if (r < 0) { r = 0; }
    gettimeofday(&video[user->video_num].tv_start, 0);
    t = r * (1000000 / video[user->video_num].fps);
    video[user->video_num].tv_start.tv_sec += (t / 1000000);
    video[user->video_num].tv_start.tv_usec = (t % 1000000);

#ifdef DEBUG
if (debug == 1)
{
  printf("Video starts over.\n");
}
#endif
  }

  frame = r;

#ifdef WITH_MMAP
  user->offset = video[user->video_num].index[r] + 4;

#if 0
  if (l > video[user->video_num].index[r])
  {
    user->offset = video[user->video_num].index[r];
  }
    else
  {
    user->offset = video[user->video_num].index[r]-l;
  }
#endif

  user->content_length =
     video[user->video_num].mem[user->offset + 0] +
    (video[user->video_num].mem[user->offset + 1] << 8) +
    (video[user->video_num].mem[user->offset + 2] << 16) +
    (video[user->video_num].mem[user->offset + 3] << 24);

  user->offset += 4;

#else
  // FIXME - Can it seek by offsets forward and does it give a speed increase?
  // l = ftell(user->in);

#if 0
  if (l > video[user->video_num].index[r])
  {
    fseek(user->in, video[user->video_num].index[r], SEEK_SET);
  }
    else
  {
    fseek(user->in, video[user->video_num].index[r] - l, SEEK_CUR);
  }
#endif

#if 0
  if (l > video[user->video_num].index[r])
  {
    lseek(user->in, video[user->video_num].index[r], SEEK_SET);
  }
    else
  {
    lseek(user->in, video[user->video_num].index[r] - l, SEEK_CUR);
  }
#endif

  lseek(user->in, video[user->video_num].index[r], SEEK_SET);

  // read_int32(user->in);
  // user->content_length = read_int32(user->in);
  int n = 0;

  while (n < 8)
  {
    int i = read(user->in, temp + n, 8 - n);
    n = n + i;
  }

  user->content_length =
    temp[4] + (temp[5] << 8)+ (temp[6] << 16) + (temp[7] << 24);
#endif

  if (user->content_length > BIGGEST_SUPPORTED_FILE ||
      user->content_length == 0)
  {
    int id = user->id;

    send_video_error(id);
    user_disconnect(user);
    return -1;
  }

  user->last_modified = time(NULL);
  user->mime_type = MIME_TYPE_JPEG;

  return frame;
}

