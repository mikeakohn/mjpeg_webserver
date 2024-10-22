/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef NETWORK_IO
#define NETWORK_IO

int send_data(int socketid, const char *message, int message_len);
int buffered_read(int id);
int send_file(int id);
int send_capture_frame(int id);
int set_socket_options(int sockfd);
int set_nonblocking(int socketid);

#endif

