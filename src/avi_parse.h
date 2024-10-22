/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef AVI_PARSE_H
#define AVI_PARSE_H

#include "video.h"

int parse_riff(FILE *in, Video *video);

#endif

