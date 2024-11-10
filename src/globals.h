/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

#ifdef ENABLE_CAPTURE
#include "capture.h"
#endif

#include "config.h"
#include "user.h"
#include "video.h"

#define VIDEO_NUM_404 404
#define VIDEO_NUM_400 400
#define BIGGEST_SUPPORTED_FILE 400000
#define BIGGEST_FILE_CHUNK 1024
#define CHUNKS_PER_SEND 8
#define NEED_HEADER_NO 0
#define NEED_HEADER_YES 1
#define REQUEST_SINGLE 0
#define REQUEST_MULTIPART 1
#define REQUEST_MULTIPART2 2

#define METHOD_GET 0
#define METHOD_POST 1
#define HANDLER_PLUGIN 2
#define HANDLER_CGI 4

#define BUFFER_SIZE 514
#define QUERY_STRING_SIZE 514   // used for plugins only for now

#ifdef WINDOWS
#define SHUT_RDWR SD_BOTH
#include <winsock2.h>
/* int gettimeofday(struct timeval *tv, struct timezone *tz); */
int gettimeofday(struct timeval *tv, int tz);
#endif

#define FOUR_OH_FOUR "<html><body><br><br><h1>404 - mjpeg_webserver FILE NOT FOUND</h1></body></html>\r\n"
#define FOUR_OH_ONE "<html><body><br><br><h1>401 - mjpeg_webserver Authorization Required</h1></body></html>\r\n"
#define VIDEO_ERROR "<html><body><br><br><h1>mjpeg_webserver Error: AVI frame too big or parse error</h1></body></html>\r\n"
#ifdef ENABLE_CGI
#define FOUR_OH_OH "<html><body><br><br><h1>400 - mjpeg_webserver Bad Request</h1></body></html>\r\n"
#endif

/* TODO -- Kill these global variables */
extern int sockfd;
//extern int port;
extern char *runas_user;
extern char *runas_group;
extern int debug;
extern uint32_t uptime;
extern uint32_t server_flags;

#endif

