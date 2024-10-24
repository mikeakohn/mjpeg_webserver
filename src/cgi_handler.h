/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef CGI_HANDLER_H
#define CGI_HANDLER_H

#ifdef ENABLE_CGI

typedef struct CgiHandler
{
  struct CgiHandler *next_handler;
  char *extension;
  char *application;
} CgiHandler;

extern CgiHandler *cgi_handler;

#endif

#endif

