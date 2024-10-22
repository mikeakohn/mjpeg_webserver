/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef SERVER_H
#define SERVER_H

#include "config.h"

#define MAX_USER_THREADS 4
#define GC_TIME 30

int server_run(Config *config);

#endif

