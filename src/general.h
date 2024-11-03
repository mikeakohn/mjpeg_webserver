/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef GENERAL_H
#define GENERAL_H

int socketdie(int socketid);
void destroy();
void broken_pipe();
void set_signals();
void message(int id, char *daMessage);
int conv_num(const char *s);
int base64_encode_(char *user_pass_64, const char *text_in);
//int base64_compare(char *text_in);

#endif

