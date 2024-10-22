/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef MIME_TYPES_H
#define MIME_TYPES_H

int get_mime_type_code(const char *s);

#define MIME_IS_CGI 32768

// This is an index into the mime_types table. The three tables
// probably should be combined somehow to make this less prone to
// error.
enum
{
  MIME_TYPE_BIN = 0,
  MIME_TYPE_HTML,
  MIME_TYPE_TEXT,
  MIME_TYPE_CSS,
  MIME_TYPE_JPEG,
  MIME_TYPE_GIF,
  MIME_TYPE_PNG,
  MIME_TYPE_JS,
  MIME_TYPE_WAP
};

extern const char *mime_types[];

#endif

