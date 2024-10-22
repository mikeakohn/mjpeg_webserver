/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef CONFIG_H
#define CONFIG_H

#define PASS64_LEN 512
#define DEFAULT_JPEG_QUALITY 80
#define DEFAULT_PORT 5555

typedef struct Config
{
  int port;
  char *htdocs_dir;
  char *index_file;
  int maxconn;
  int minconn;
  int max_idle_time;
  char user_pass_64[PASS64_LEN];
  int jpeg_quality;
  int frame_rate;
} Config;

void config_init(Config *config, int argc, char *argv[]);
void config_destroy(Config *config);
void config_dump(Config *config);
void config_read(Config *config, const char *config_dir);
void config_set_runas(Config *config);

#endif

