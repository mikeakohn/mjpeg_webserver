/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef JPEG_LIB
#include "jpeg_compress.h"
#endif

#ifdef ENABLE_CAPTURE
#include "capture.h"
#endif

#include "config.h"
#include "server.h"

#if 0
#include "avi_play.h"
#include "general.h"
#include "globals.h"
#include "http_headers.h"
#include "functions.h"
#include "fileio.h"
#include "mime_types.h"
#include "network_io.h"
#include "url_utils.h"
#include "user.h"
#endif

int main(int argc, char *argv[])
{
#ifdef WINDOWS
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(1, 1);
  int Win32isStupid;

  Win32isStupid = WSAStartup(wVersionRequested, &wsaData);

  if (Win32isStupid)  /* Shouldn't this always be true? :) */
  {
    printf("Winsock can't start.\n");
    exit(1);
  }
#endif

  Config config;

  config_init(&config, argc, argv);
  config_dump(&config);

  server_run(&config);

  config_destroy(&config);

  return 0;
}
