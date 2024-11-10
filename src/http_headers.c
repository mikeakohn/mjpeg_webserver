/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_CAPTURE
#include "capture.h"
#endif

#include "file_io.h"
#include "functions.h"
#include "general.h"
#include "globals.h"
#include "http_headers.h"
#include "mime_types.h"
#include "network_io.h"
#include "user.h"
#include "version.h"

#if 0
HTTP/1.1 200 OK
Date: Sat, 08 Oct 2005 01:51:33 GMT
Server: Apache/2.0.52 (Unix) PHP/5.0.3 mod_perl/2.0.0 Perl/v5.8.6
Last-Modified: Sat, 08 Oct 2005 01:51:05 GMT
ETag: "98469-e0fb-40294641ed440"
Accept-Ranges: bytes
Content-Length: 57595
Connection: close
Content-Type: image/jpeg
#endif

int send_header(int id)
{
  char temp[256];

  // struct tm mod_tm;
  // gmtime_r(&users[id]->last_modified,&mod_tm);

  message(id,
    "HTTP/1.1 200 OK\r\n"
    "Server: " VERSION "\r\n"
    "Cache-Control: no-cache\r\n"
    "Pragma: no-cache\r\n"
    "Last-Modified: Wed, 29 May 1974 07:00:00 GMT\r\n"
    "Connection: Keep-Alive\r\n");

  // snprintf(temp, sizeof(temp),
  //  "Last-Modified: %s\r\n", ctime(&users[id]->last_modified));

#if 0
  snprintf(temp, sizeof(temp),
    "Last-Modified: %s, %02d %s %d %02d:%02d:%02d GMT\r\n",
    days[mod_tm.tm_wday],
    mod_tm.tm_mday,
    months[mod_tm.tm_mon],
    mod_tm.tm_year+1900,
    mod_tm.tm_hour,
    mod_tm.tm_min,
    mod_tm.tm_sec);

printf("%s\n",temp);
  message(id,temp);
#endif

  snprintf(temp, sizeof(temp),
    "Content-Length: %d\r\nContent-Type: %s\r\n\r\n",
    users[id]->content_length,
    mime_types[users[id]->mime_type]);

  message(id, temp);

  // users[id]->need_header = NEED_HEADER_NO;

  return 0;
}

#ifdef ENABLE_CGI
int send_header_cgi(int id)
{

  message(id,
    "HTTP/1.1 200 OK\r\n"
    "Server: " VERSION "\r\n"
    "Cache-Control: no-cache\r\n"
    "Pragma: no-cache\r\n"
    "Last-Modified: Wed, 29 May 1974 07:00:00 GMT\r\n"
    "Connection: Close\r\n");

  // Biggest cgi can be only 24 megs.
  users[id]->content_length = 1 << 24;

  return 0;
}
#endif

#ifdef ENABLE_PLUGINS
int send_header_plugin(int id)
{

  message(id,
    "HTTP/1.1 200 OK\r\n"
    "Server: " VERSION "\r\n"
    "Cache-Control: no-cache\r\n"
    "Pragma: no-cache\r\n"
    "Last-Modified: Wed, 29 May 1974 07:00:00 GMT\r\n");

  // Biggest cgi can be only 24 megs.
  users[id]->content_length = 1 << 24;

  return 0;
}
#endif

int send_header_multipart(int id)
{
  char temp[512];

  message(id,
    "HTTP/1.0 200 OK\r\n"
    "Cache-Control: no-cache\r\n"
    "Pragma: no-cache\r\n"
    "Expires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
    "Connection: Close\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=--myboundary\r\n\r\n");

  snprintf(temp, sizeof(temp),
    "--myboundary\r\n"
    "Content-Type: image/jpeg\r\n"
    "Content-Length: %d\r\n\r\n",
    users[id]->content_length);

  message(id, temp);

  //users[id]->need_header = NEED_HEADER_YES;
  //users[id]->need_header = NEED_HEADER_NO;

  users[id]->request_type=REQUEST_MULTIPART2;

  return 0;
}

int send_error(int id, const char *short_error, const char *error, int len)
{
  char temp[1024];

  snprintf(temp, sizeof(temp),
    "HTTP/1.1 %s\r\n"
    "Server: " VERSION "\r\n"
    "Connection: Keep-Alive\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %d\r\n\r\n", short_error, len);

  message(id, temp);

  // send_data(users[id]->socketid,FOUR_OH_FOUR,sizeof(FOUR_OH_FOUR));
  send_data(users[id]->socketid, error, len);

  users[id]->video_num = -1;
  users[id]->state = STATE_IDLE;

  return 0;
}

int send_401(int id)
{
  char temp[32];

  message(id,
    "HTTP/1.1 401 Authorization Required\r\n"
    "Server: " VERSION "\r\n"
    "Connection: Keep-Alive\r\n"
    "WWW-Authenticate: Basic realm \"mjpeg_webserver\"\r\n"
    "Content-Type: text/html\r\n");

  snprintf(temp, sizeof(temp), "Content-Length: %d\r\n\r\n", (int)sizeof(FOUR_OH_ONE));
  message(id, temp);

  send_data(users[id]->socketid,FOUR_OH_ONE,sizeof(FOUR_OH_ONE));

  users[id]->video_num = -1;
  users[id]->state = STATE_IDLE;

  if (users[id]->in != -1)
  {
    // fclose(users[id]->in);
    file_close(users[id]);
  }

  return 0;
}

int send_video_error(int id)
{
  char temp[32];

  message(id,
    "HTTP/1.1 200 OK\r\n"
    "Server: " VERSION "\r\n"
    "Connection: Keep-Alive\r\n"
    "Content-Type: text/html\r\n");

  snprintf(temp, sizeof(temp), "Content-Length: %d\r\n\r\n", (int)sizeof(VIDEO_ERROR));
  message(id, temp);

  send_data(users[id]->socketid, VIDEO_ERROR, sizeof(VIDEO_ERROR));

  users[id]->video_num = -1;
  users[id]->state = STATE_IDLE;

  return 0;
}

