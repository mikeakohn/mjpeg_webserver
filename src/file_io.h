/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef FILEIO_H
#define FILEIO_H

#include "config.h"
#include "user.h"

int file_open(User *user, Config *config, char *filename);
int file_close(User *user);

int read_int32(FILE *in);
int read_int16(FILE *in);

int read_chars(FILE *in, char *s, int count);

#endif

