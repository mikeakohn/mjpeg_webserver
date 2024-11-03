/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WINDOWS
#include <signal.h>
#endif

#include "general.h"
#include "set_signals.h"

#ifndef WINDOWS
void broken_pipe()
{
#ifdef DEBUG
  if (debug == 1) { printf("mjpeg_webserver: Pipe Broken\n"); }
#endif

  set_signals();
}

void other_signal()
{
#ifdef DEBUG
  if (debug == 1) { printf("Signal thrown\n"); }
#endif

  set_signals();
}

void set_signals()
{
  signal(SIGPIPE, broken_pipe);
  signal(SIGURG,  other_signal);
  signal(SIGIO,   other_signal);
  signal(SIGHUP,  other_signal);

  signal(SIGINT,  destroy);
  signal(SIGTERM, destroy);
}
#endif

