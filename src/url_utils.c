/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "globals.h"
#include "functions.h"
#include "user.h"

// URL decode without allocating any new memory.

int url_decode(uint8_t *s)
{
  int sptr, dptr;
  int c, t;

  sptr = 0;
  dptr = 0;

  while (s[sptr] != 0)
  {
    if (s[sptr] == '%')
    {
      c = 0;
      for (t = 0; t < 2; t++)
      {
        sptr++;

        if (s[sptr]>='0' && s[sptr]<='9')
        {
          c = (c << 4) + (s[sptr] - '0');
        }
          else
        if (tolower(s[sptr]) >= 'a' && tolower(s[sptr]) <= 'f')
        {
          c = (c << 4) + (tolower(s[sptr]) - 'a');
        }
          else
        {
          s[dptr] = 0;
          return -1;
        }
      }

      if (c >= ' ') { s[dptr] = c; }
    }
      else
    {
      s[dptr] = s[sptr];
    }

    dptr++;
    sptr++;
  }

  s[dptr] = 0;

  return 0;
}

char *get_querystring(char *filename)
{
  int ptr;

  ptr = 0;

  while(filename[ptr] != '?' && filename[ptr] != 0) { ptr++; }

  if (filename[ptr] == 0) { return filename + ptr; }

  filename[ptr] = 0;

  return filename + ptr + 1;
}

int parse_querystring(User *user, char *filename, struct alias_t *curr_alias)
{
  int ptr, r;
  char name[256];
  char value[256];
  int state;

  ptr = 0;

  while (filename[ptr] != '?' && filename[ptr] != 0) { ptr++;}

  if (filename[ptr] == 0) { return -1; }

  filename[ptr] = 0;
  ptr++;

  r = 0;
  state=0;

  while (1)
  {
    if (state == 0)
    {
      if (filename[ptr] == 0) { break; }

      if (filename[ptr]=='=')
      {
        name[r] = 0;
        r = 0;
        state = 1;
        ptr++;

        continue;
      }

      name[r++] = filename[ptr++];
    }
      else
    {
      if (filename[ptr] == '&' || filename[ptr] == 0)
      {
        value[r] = 0;
        r = 0;
        state = 0;

        if (curr_alias->port_param != 0 &&
            strcasecmp(name,curr_alias->port_param) == 0)
        {
          // This might be a problem.
          user->video_num = atoi(value) - 1; }
          else
        if (curr_alias->fps_param != 0 &&
            strcasecmp(name,curr_alias->fps_param) == 0)
        {
          user->frame_rate = atoi(value);
        }
#ifdef V4L2
          else
        if (curr_alias->comp_param != 0 &&
            strcasecmp(name, curr_alias->comp_param) == 0)
        {
          user->jpeg_quality = atoi(value);
        }
#endif

        if (filename[ptr] == 0) { break; }
        ptr++;
        continue;
      }

      value[r++] = filename[ptr++];
    }
  }

  if (user->video_num < 0) { user->video_num = VIDEO_NUM_404; }

  return 0;
}

int check_valid_file(const char *s)
{
  int ptr, c;

  c = 0;
  ptr = 0;

  while (s[ptr] != 0)
  {
    if (s[ptr] == '.')
    {
      c++;
    }
      else
    {
      c = 0;
    }

    if (s[ptr] == '~') { return -1; }
    if (c == 2) { return -1; }

    ptr++;
  }

  return 0;
}

