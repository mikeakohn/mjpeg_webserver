/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef URL_UTILS_H
#define URL_UTILS_H

#include <stdint.h>

#include "alias.h"
#include "user.h"

int url_decode(uint8_t *s);
char *get_querystring(char *filename);
int parse_querystring(User *user, char *filename, Alias *curr_alias);
int check_valid_file(const char *s);

#endif

