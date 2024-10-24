/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef ALIAS_H
#define ALIAS_H

typedef struct Alias
{
  struct Alias *next_alias;
  int port;
  int port_start;
  int type;
  int url_len;
  char *url;
  char *port_param;
  char *size_param;
  char *comp_param;
  char *fps_param;
} Alias;

extern Alias *alias;

#endif

