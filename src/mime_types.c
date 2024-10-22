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

#include "mime_types.h"
#include "globals.h"

const char *mime_types[] =
{
  "application/octet-stream",
  "text/html",
  "text/plain",
  "text/css",
  "image/jpeg",
  "image/gif",
  "image/png",
  "application/x-javascript",
  "text/vnd.wap.wml",
  NULL
};

// FIXME: This should be combined with mime_type_code[] table.
static const char *extensions[] =
{
  "html", "htm",
  "text", "txt",
  "css",
  "jpeg", "jpg",
  "gif",
  "png",
  "js",
  "wml",
  "shtml",
  NULL
};

static const int mime_type_code[] =
{
  MIME_TYPE_HTML, MIME_TYPE_HTML,
  MIME_TYPE_TEXT, MIME_TYPE_TEXT,
  MIME_TYPE_CSS,
  MIME_TYPE_JPEG, MIME_TYPE_JPEG,
  MIME_TYPE_GIF,
  MIME_TYPE_PNG,
  MIME_TYPE_JS,
  MIME_TYPE_WAP,
  MIME_TYPE_HTML,
  0
};

int get_mime_type_code(const char *s)
{
#ifdef ENABLE_CGI
  struct cgi_handler_t *curr_handler;
#endif
  int l, t;

  l = strlen(s) - 1;

  while (l > 0)
  {
    if (s[l] == '.') { break; }
    l--;
  }

  if (s[l] != '.') { return 0; }

  l++;

  t = 0;
  while (extensions[t] != 0)
  {
    if (strcasecmp(s + l, extensions[t]) == 0)
    {
      return mime_type_code[t];
    }

    t++;
  }

#ifdef ENABLE_CGI
  t = MIME_IS_CGI;
  curr_handler = cgi_handler;

  while (curr_handler != 0)
  {
    if (strcasecmp(s + l, curr_handler->extension) == 0) { return t; }

    curr_handler = curr_handler->next_handler;
    t++;
  }
#endif

  return 0;
}

