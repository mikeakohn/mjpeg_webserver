/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include "globals.h"

int send_header(int id);
#ifdef ENABLE_CGI
int send_header_cgi(int id);
#endif
#ifdef ENABLE_PLUGINS
int send_header_plugin(int id);
#endif
int send_header_multipart(int id);
int send_error(int id, const char *short_error, const char *error, int len);
int send_401(int id);
int send_video_error(int id);

#endif

