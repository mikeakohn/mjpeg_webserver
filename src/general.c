/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef WINDOWS
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#endif
#include <errno.h>
#include <time.h>
#include <ctype.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#ifdef WITH_MMAP
#ifndef WINDOWS
#include <sys/mman.h>
#endif
#endif

#include "alias.h"
#include "avi.h"
#include "cgi_handler.h"
#include "config.h"
#include "general.h"
#include "globals.h"
#include "functions.h"
#include "network_io.h"
#include "plugin.h"
#include "user.h"

#ifdef WINDOWS
int gettimeofday(struct timeval *tv, int tz)
{
  SYSTEMTIME currTime;

  GetSystemTime(&currTime);

  tv->tv_sec =
    (currTime.wHour * 3600) +
    (currTime.wMinute * 60) +
    (currTime.wSecond);

  tv->tv_usec = currTime.wMilliseconds * 1000;

/* printf("%d %d\n",tv->tv_sec,tv->tv_usec); */

  return 0;
}
#endif

int socketdie(int socketid)
{
#ifdef WINDOWS
  closesocket(socketid);
#else
  close(socketid);
#endif

  return 0;
}

void destroy()
{
#ifdef ENABLE_CGI
  CgiHandler *next_handler;
#endif
#ifdef ENABLE_PLUGIN
  Plugin *next_plugin;
#endif
  Alias *next_alias;
  int r;

  //if (htdocs_dir != 0) { free(htdocs_dir); }

  for (r = 0; r < video_count; r++)
  {
#ifdef ENABLE_CAPTURE
    if (video[r].capture_info != 0) { free(video[r].capture_info); }
#endif
#ifdef WITH_MMAP
#ifndef WINDOWS
    munmap(video[r].mem, (size_t)video[r].file_len);
    close(video[r].fd);
#else
    UnmapViewOfFile(video[r].mem);
    CloseHandle(video[r].mem_handle);
    CloseHandle(video[r].fd);
#endif
#endif
    free(video[r].filename);
  }

  while(alias != 0)
  {
    next_alias = alias->next_alias;

    if (alias->url != NULL)        { free(alias->url); }
    if (alias->port_param != NULL) { free(alias->port_param); }
    if (alias->size_param != NULL) { free(alias->size_param); }
    if (alias->comp_param != NULL) { free(alias->comp_param); }
    if (alias->fps_param != NULL)  { free(alias->fps_param); }
    free(alias);

    alias = next_alias;
  }

#ifdef ENABLE_CGI
  while (cgi_handler != 0)
  {
    next_handler = cgi_handler->next_handler;

    if (cgi_handler->extension != NULL)   { free(cgi_handler->extension); }
    if (cgi_handler->application != NULL) { free(cgi_handler->application); }
    free(cgi_handler);

    cgi_handler = next_handler;
  }
#endif

#ifdef ENABLE_PLUGIN
  while (plugin != NULL)
  {
    next_plugin = plugin->next_plugin;

    if (plugin->alias != NULL) { free(plugin->alias);}

    dlclose(dlhandle);
    free(plugin);

    plugin = next_plugin;
  }
#endif

#ifdef DEBUG
  if (debug == 1)
  {
    printf("mjpeg_webserver terminated.\n");
  }
#endif

  exit(0);
}

void message(int id, char *message_string)
{
  if (users[id]->inuse == 1)
  {
    if (send_data(users[id]->socketid, message_string, strlen(message_string)) <= 0)
    {
      users[id]->inuse = 2;
      user_disconnect(users[id]);
    }
  }
}

int conv_num(const char *s)
{
  int n, r = 0;

  n = 0;

  while (r < 20)
  {
    if (s[r] < '0' || s[r] > '9') { break; }
    n = (n * 10) + (s[r] - '0');
    r++;
  }

  if (r == 0) { return -1; }

  return n;
}

#if 0
void setup(int argc, char *argv[])
{
  int r;

  port = DEFAULT_PORT;
  debug = 0;
  alias = 0;
#ifdef ENABLE_CGI
  cgi_handler = 0;
#endif
#ifdef ENABLE_PLUGIN
  plugin = 0;
#endif
  user_pass_64[0] = 0;
  htdocs_dir = NULL;
  jpeg_quality = DEFAULT_JPEG_QUALITY;

  int config_was_read = 0;

  for (r = 1; r < argc; r++)
  {
    if (strcasecmp(argv[r], "-f") == 0)
    {
      config_read(argv[++r]);
      config_was_read = 1;
      break;
    }
  }

  if (config_was_read == 0) { config_read("mjpeg_webserver.conf"); }

  uptime = time(NULL);

  for (r = 1; r < argc; r++)
  {
    if (strcmp(argv[r], "-p") == 0)
    {
      port = atoi(argv[++r]);
    }
      else
    if (strcasecmp(argv[r], "-d") == 0)
    {
      debug = 1;
    }
  }

#ifdef DEBUG
  if (debug == 1)
  {
    printf("           Port: %d\n", port);
    printf("\n");
  }
#endif

}
#endif

int base64_encode(char *user_pass_64, const char *text_in)
{
  int t,l,ptr;
  int holding,bitptr;
  char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

  l = strlen(text_in);
  t = 0;

  ptr = 0;

  holding = 0;
  bitptr = 0;

  while (t < l)
  {
    if (ptr >= PASS64_LEN)
    {
      user_pass_64[0] = 0;
      return -1;
    }

    if (bitptr < 6)
    {
      holding = (holding << 8) + text_in[t++];
      bitptr = bitptr + 8;
    }

    bitptr = bitptr - 6;
    user_pass_64[ptr++] = base64[holding >> bitptr];
    holding = holding & ((1 << bitptr) - 1);
  }

  while (bitptr > 0)
  {
    if (ptr >= PASS64_LEN) { return -1; }

    if (bitptr >= 6)
    {
      user_pass_64[ptr++] = base64[holding >> (bitptr - 6)];
    }
      else
    {
      user_pass_64[ptr++] = base64[holding << (6 - bitptr)];
    }

    bitptr = bitptr - 6;
    holding = holding & ((1 << bitptr) - 1);
  }

  if ((ptr % 4) != 0)
  {
    l = 4 - (ptr % 4);

    if (ptr >= PASS64_LEN - l) { return -1; }

    for (t = 0; t < l; t++)
    {
      user_pass_64[ptr++] = '=';
    }
  }

  if (ptr >= PASS64_LEN) { return -1; }
  user_pass_64[ptr++] = 0;

  return 0;
}

