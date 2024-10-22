/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef USER_H
#define USER_H

#include <stdint.h>
#include <netdb.h>

#include "config.h"

#define BUFFER_SIZE 514

#define STATE_IDLE 0
#define STATE_HEADERS 1
#define STATE_SEND_FILE 2

/*

User Flags

0: authentication okay
1: did authenication once

*/

typedef struct User
{
  uint8_t location[64];
  char out_buffer[BUFFER_SIZE];
  uint8_t in_buffer[BUFFER_SIZE];
  int id, r;
  int logontime, idletime;
  char inuse;
  int socketid;
  int in_ptr, in_len;
  int buffer_ptr;
  int curr_frame;
  FILE *pin;
  int in;
#ifdef WITH_MMAP
  long offset;
#endif
  int video_num;
  int mime_type;
  long content_length;
  time_t last_modified;
  int state;
  int need_header;
  int request_type;
  int last_frame;
  uint32_t flags;
  int frame_rate;
  int method;
#ifdef ENABLE_PLUGINS
  char querystring[QUERY_STRING_SIZE];
  struct plugin_t *plugin;
#endif
#ifdef ENABLE_CAPTURE
  uint8_t *jpeg;
  int jpeg_len;
  int jpeg_quality;
  int jpeg_ptr;
#endif
} User;

void user_init(User *user, int id, int socketid);
void user_destroy(User *user);
int user_connect(Config *config, int socketid, struct sockaddr_in *cli_addr);
void user_disconnect(User *user);

#endif

