/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef WINDOWS
#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <dlfcn.h>
#endif
#include <errno.h>
#include <time.h>
#include <ctype.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#ifdef WITH_MMAP
#ifndef WINDOWS
#include <sys/mman.h>
#endif
#endif

#include "avi.h"
#include "avi_play.h"
#include "config.h"
#include "general.h"
#include "globals.h"
#include "functions.h"

void config_init(Config *config, int argc, char *argv[])
{
  int r;

  config->port = DEFAULT_PORT;
  config->jpeg_quality = DEFAULT_JPEG_QUALITY;
  config->htdocs_dir = NULL;
  config->index_file = NULL;
  config->minconn = 10;
  config->maxconn = 50;
  config->max_idle_time = 60;
  memset(config->user_pass_64, 0, sizeof(config->user_pass_64));
  config->frame_rate = 30;

  debug = 0;
  alias = 0;
#ifdef ENABLE_CGI
  cgi_handler = 0;
#endif
#ifdef ENABLE_PLUGIN
  plugin = 0;
#endif

  int config_was_read = 0;

  for (r = 1; r < argc; r++)
  {
    if (strcasecmp(argv[r], "-f") == 0)
    {
      config_read(config, argv[++r]);
      config_was_read = 1;
      break;
    }
  }

  if (config_was_read == 0) { config_read(config, "mjpeg_webserver.conf"); }

  for (r = 1; r < argc; r++)
  {
    if (strcmp(argv[r], "-p") == 0)
    {
      config->port = atoi(argv[++r]);
    }
      else
    if (strcasecmp(argv[r], "-d") == 0)
    {
      debug = 1;
    }
  }
}

void config_destroy(Config *config)
{
  free(config->htdocs_dir);
  free(config->index_file);
}

void config_dump(Config *config)
{
  printf(" -- config_dump() --\n");
  printf("         port: %d\n", config->port);
  printf("   htdocs_dir: %s\n", config->htdocs_dir);
  printf("   index_file: %s\n", config->index_file);
  printf(" jpeg_quality: %d\n", config->jpeg_quality);
  printf("      minconn: %d\n", config->minconn);
  printf("      maxconn: %d\n", config->maxconn);
  printf("max_idle_time: %d\n", config->max_idle_time);
  printf("   frame_rate: %d\n", config->frame_rate);
}

#ifdef ENABLE_CAPTURE
static void parse_size(char *s, int *width, int *height)
{
  int ptr, r, k;

  ptr = 0;
  r = 0;
  k = 0;

  while (1)
  {
    if (s[ptr] == 0 && k == 0) { break; }

    if (s[ptr] == 'x' || s[ptr] == ',' || s[ptr] == 0)
    {
      s[ptr++] = 0;

      if (k == 0)
      {
        *width = atoi(s + r);
      }
        else
      {
        *height = atoi(s + r);
      }

      k++;
      if (k == 2) { break; }
      r = ptr;
      continue;
    }

    ptr++;
  }
}
#endif

static int gettoken(FILE *in, char *token, int token_len)
{
  int ch, ptr;

  ptr = 0;
  token[0] = 0;

  while (1)
  {
    ch = getc(in);

    if (ch == '#' && ptr == 0)
    {
      while (1)
      {
        ch = getc(in);
        if (ch == '\n' || ch == '\r' || ch == EOF) { break; }
      }

      continue;
    }

    if (ch == ';' || ch == '{' || ch == '}')
    {
      if (ptr == 0)
      {
        token[ptr++] = ch;
        break;
      }

      ungetc(ch, in);
      break;
    }

    if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t' || ch == EOF)
    {
      if (ptr != 0)  { break; }
      if (ch == EOF) { return -1; }
      continue;
    }

    token[ptr++] = ch;
    if (ptr >= token_len - 1) { break; }
  }

  token[ptr] = 0;

  return 0;
}

#ifdef ENABLE_CGI
static int parse_handler(FILE *in)
{
  struct cgi_handler_t *curr_handler, *temp_handler;
  char token[2048];

  curr_handler = (struct cgi_handler_t *)malloc(sizeof(struct cgi_handler_t));

  if (cgi_handler == NULL)
  {
    cgi_handler = curr_handler;
  }
    else
  {
    temp_handler = cgi_handler;

    while(temp_handler->next_handler != NULL)
    {
      temp_handler = temp_handler->next_handler;
    }

    temp_handler->next_handler = curr_handler;
  }

  memset(curr_handler, 0, sizeof(struct cgi_handler_t));

  gettoken(in, token, sizeof(token));
  const int length = strlen(token) + 1;
  curr_handler->extension = (char *)malloc(length);
  snprintf(curr_handler->extension, length, "%s", token);

  gettoken(in, token, sizeof(token));
  if (strcmp(token, ";") == 0) { return 0;  }
  if (strcmp(token, "{") != 0) { return -1; }

  while (1)
  {
    if (gettoken(in, token, sizeof(token)) != 0)
    {
      printf("Parse error in alias, expected '}'\n");
      return -1;
    }

    if (strcmp(token, "}") == 0) { break; }

    if (strcasecmp(token,"application") == 0)
    {
      gettoken(in, token, sizeof(token));
      curr_handler->application = (char *)malloc(strlen(token) + 1);
      strcpy(curr_handler->application, token);
    }
  }

  return 0;
}
#endif

#ifdef ENABLE_PLUGINS

static int parse_plugin(FILE *in)
{
  struct plugin_t *curr_plugin, *temp_plugin;
  mjpeg_webserver_plugin_init_t mjpeg_webserver_plugin_init = NULL;
  char token[2048];

  curr_plugin = (struct plugin_t *)malloc(sizeof(struct plugin_t));

  if (plugin == 0)
  {
    plugin = curr_plugin;
  }
    else
  {
    temp_plugin = plugin;

    while(temp_plugin->next_plugin != NULL)
    {
      temp_plugin = temp_plugin->next_plugin;
    }

    temp_plugin->next_plugin = curr_plugin;
  }

  memset(curr_plugin, 0, sizeof(struct plugin_t));

  gettoken(in, token, sizeof(token));
  curr_plugin->alias = (char *)malloc(strlen(token) + 1);
  strcpy(curr_plugin->alias, token);
  curr_plugin->alias_len = strlen(token);

  gettoken(in, token, sizeof(token));
  if (strcmp(token, "{") != 0) return -1;

  while (1)
  {
    if (gettoken(in, token, sizeof(token)) != 0)
    {
      printf("Parse error in plugin, expected '}'\n");
      return -1;
    }

    if (strcmp(token,"}") == 0) { break;}

    if (strcasecmp(token, "library") == 0)
    {
      gettoken(in, token, sizeof(token));
      curr_plugin->dlhandle = dlopen(token, RTLD_LAZY);

      if (curr_plugin->dlhandle == NULL)
      {
        printf("Error opening up library %s\n", token);
        continue;
      }

      mjpeg_webserver_plugin_init =
        (mjpeg_webserver_plugin_init_t)dlsym(curr_plugin->dlhandle, "mjpeg_webserver_plugin_init");
      curr_plugin->get =
        (mjpeg_webserver_plugin_get_t)dlsym(curr_plugin->dlhandle, "mjpeg_webserver_plugin_get");
      curr_plugin->post =
        (mjpeg_webserver_plugin_post_t)dlsym(curr_plugin->dlhandle, "mjpeg_webserver_plugin_post");
    }
  }

  if (mjpeg_webserver_plugin_init == NULL ||
      curr_plugin->get == NULL ||
      curr_plugin->post == NULL)
  {
    printf("Error loading plugin\n");
    curr_plugin->alias[0] = 0;
  }
    else
  {
    mjpeg_webserver_plugin_init();
  }

  return 0;
}

#endif

static int parse_alias(FILE *in)
{
  struct alias_t *curr_alias, *temp_alias;
  char token[2048];
  int r;

  curr_alias = (struct alias_t *)malloc(sizeof(struct alias_t));

  if (alias == NULL)
  {
    alias = curr_alias;
  }
    else
  {
    temp_alias = alias;
    while (temp_alias->next_alias != NULL)
    {
      temp_alias = temp_alias->next_alias;
    }

    temp_alias->next_alias = curr_alias;
  }

  memset(curr_alias, 0, sizeof(struct alias_t));

  gettoken(in, token, sizeof(token));
  curr_alias->url_len = strlen(token);
  curr_alias->url = (char *)malloc(strlen(token) + 1);
  strcpy(curr_alias->url, token);

  gettoken(in, token, sizeof(token));

  if (strcmp(token, "{") != 0)
  {
    printf("Parse error in alias, expected '{' and got '%s'\n", token);
    return -1;
  }

  while (1)
  {
    if (gettoken(in, token, sizeof(token)) != 0)
    {
      printf("Parse error in alias, expected '}' and got '%s'\n",token);
      return -1;
    }

    if (strcmp(token, "}") == 0) { break;}

    if (strcasecmp(token, "port") == 0)
    {
      gettoken(in, token, sizeof(token));
      r = conv_num(token);

      if (r != -1)
      {
        curr_alias->port = r;
      }
        else
      {
        curr_alias->port_param = (char *)malloc(strlen(token) + 1);
        strcpy(curr_alias->port_param, token);
      }
    }
      else
    if (strcasecmp(token, "type") == 0)
    {
      gettoken(in,token,sizeof(token));

      if (strcasecmp(token,"single") == 0)
      {
        curr_alias->type = 0;
      }
        else
      if (strcasecmp(token,"stream") == 0)
      {
        curr_alias->type=1;
      }
    }
      else
    if (strcasecmp(token, "size") == 0 && curr_alias->size_param == 0)
    {
      gettoken(in, token, sizeof(token));
      curr_alias->size_param = (char *)malloc(strlen(token) + 1);
      strcpy(curr_alias->size_param, token);
    }
      else
    if (strcasecmp(token,"comp") == 0 && curr_alias->comp_param == 0)
    {
      gettoken(in, token, sizeof(token));
      curr_alias->comp_param = (char *)malloc(strlen(token) + 1);
      strcpy(curr_alias->comp_param, token);
    }
      else
    if (strcasecmp(token,"frame_rate") == 0 && curr_alias->fps_param == 0)
    {
      gettoken(in, token, sizeof(token));
      curr_alias->fps_param = (char *)malloc(strlen(token) + 1);
      strcpy(curr_alias->fps_param, token);
    }
      else
    {
      printf("Error in conf '%s'\n", token);
      return -1;
    }
  }

  return 0;
}

#ifdef ENABLE_CAPTURE
static int parse_capture(FILE *in)
{
  char token[1024];
  char value[1024];
  char video_dev[1024];
  int i;

  gettoken(in, video_dev, sizeof(video_dev));
  gettoken(in, token, sizeof(token));

  if (strcmp(token, "{") != 0)
  {
    printf("Parse error in alias, expected '{' and got '%s'\n", token);
    return -1;
  }

  memset(&video[video_count], 0, sizeof(Video));
  video[video_count].capture_info =
    (struct capture_info_t *)malloc(sizeof(struct capture_info_t));

  video[video_count].capture_info->width = 352;
  video[video_count].capture_info->height = 240;
  video[video_count].capture_info->channel = -1;
#ifdef V4L
  video[video_count].capture_info->format = V4L2_STD_NTSC_M;
#endif

  while (1)
  {
    if (gettoken(in, token, sizeof(token)) == -1) { return -1; }

    if (strcmp(token, "}") ==0) { break; }

    gettoken(in, value, sizeof(value));

    if (strcmp(token, "size") == 0)
    {
      parse_size(
        value,
        &video[video_count].capture_info->width,
        &video[video_count].capture_info->height);
    }
      else
    if (strcmp(token, "max_fps") == 0)
    {
      video[video_count].capture_info->max_fps = atoi(value);
    }
      else
    if (strcmp(token, "channel") == 0)
    {
      video[video_count].capture_info->channel = atoi(value);
    }
      else
    if (strcmp(token, "format") == 0)
    {
#ifdef V4L
      if (strcasecmp(value, "pal") == 0)
      {
        video[video_count].capture_info->format = V4L2_STD_PAL_M;
      }
        else
      if (strcasecmp(value, "ntsc") == 0)
      {
        video[video_count].capture_info->format = V4L2_STD_NTSC_M;
      }
        else
      {
        video[video_count].capture_info->format = atoi(value);
      }
#endif
    }
  }

  printf("%s  %dx%d  max_fps=%d\n",
    video_dev,
    video[video_count].capture_info->width,
    video[video_count].capture_info->height,
    video[video_count].capture_info->max_fps);

  i = open_capture(video[video_count].capture_info, video_dev);

  if (i == 0)
  {
    video_count++;
  }
    else
  {
    printf("Couldn't open capture for device %s (error %d)\n", video_dev, i);
    free(video[video_count].capture_info);
  }

  return 0;
}
#endif

#ifndef WINDOWS
void config_set_runas(Config *config)
{
  struct passwd *pw;
  struct group *gr;

  if (runas_user != NULL)
  {
    pw = getpwnam(runas_user);

    if (pw == 0)
    {
      printf("No such user %s\n", runas_user);
    }
      else
    {
      if (runas_group != NULL)
      {
        gr = getgrnam(runas_group);

        if (gr == 0)
        {
          printf("No such group %s\n", runas_group);
        }
          else
        if (setgid(gr->gr_gid) != 0)
        {
          printf("Couldn't set group %s\n", runas_group);
        }

        free(runas_group);
        runas_group = 0;
      }

      if (setuid(pw->pw_uid) != 0)
      {
        printf("Couldn't set user to %s\n", runas_user);
      }
    }

    free(runas_user);
    runas_user = NULL;
  }
}

#endif

#if !defined(ENABLE_CGI) || !defined(ENABLE_PLUGINS) || !defined(ENABLE_CAPTURE)
static void skip_block(FILE *in)
{
  char token[1024];
  int brace_count = 0;

  while (gettoken(in, token, sizeof(token)) == 0)
  {
    if (strcmp(token,"{") == 0)
    {
      brace_count++;
    }
      else
    if (strcmp(token,"}") == 0)
    {
      brace_count--;
      if (brace_count == 0) { return; }
    }
      else
    if (strcmp(token,";") == 0 && brace_count == 0)
    {
      return;
    }
  }
}
#endif

void config_read(Config *config, const char *config_dir)
{
  FILE *in;
  char token[1024];
  char username[512];
  char password[256];
#ifndef WINDOWS
  int ch;
#endif

  username[0] = 0;
  password[0] = 0;

  in = fopen(config_dir, "rb");

  if (in == 0)
  {
    printf("No config file found.  Going with defaults.\n");
    return;
  }

  while (gettoken(in, token, sizeof(token)) == 0)
  {
    if (strcasecmp(token, "filename") == 0)
    {
      gettoken(in, token, sizeof(token));
      avi_init(token);
    }
      else
    if (strcasecmp(token, "port") == 0)
    {
      gettoken(in, token, sizeof(token));
      config->port = atoi(token);
    }
      else
    if (strcasecmp(token, "maxconn") == 0)
    {
      gettoken(in, token, sizeof(token));
      config->maxconn = atoi(token);
    }
      else
    if (strcasecmp(token, "minconn") == 0)
    {
      gettoken(in, token, sizeof(token));
      config->minconn = atoi(token);
    }
      else
    if (strcasecmp(token, "max_idle_time") == 0)
    {
      gettoken(in, token, sizeof(token));
      config->max_idle_time = atoi(token);
    }
      else
    if (strcasecmp(token, "htdocs_dir") == 0)
    {
      gettoken(in, token, sizeof(token));
      int length = strlen(token) + 1;
      config->htdocs_dir = (char *)malloc(length);
      snprintf(config->htdocs_dir, length, "%s", token);
    }
      else
    if (strcasecmp(token, "index_file") == 0)
    {
      gettoken(in, token, sizeof(token));
      int length = strlen(token) + 1;
      config->index_file = (char *)malloc(length);
      snprintf(config->index_file, length, "%s", token);
    }
      else
    if (strcasecmp(token, "username") == 0)
    {
      gettoken(in, username, sizeof(username));
    }
      else
    if (strcasecmp(token, "password") == 0)
    {
      gettoken(in, password, sizeof(password));
    }
      else
    if (strcasecmp(token, "runas") == 0)
    {
      gettoken(in, token, sizeof(token));

#ifdef WINDOWS
      printf("runas is not supported on Windows\n");
#else
      ch = 0;
      while (token[ch] != ':' && token[ch] != 0) { ch++; }

      if (token[ch] == ':')
      {
        token[ch++] = 0;
      }
        else
      {
        ch++;
        token[ch] = 0;
      }

      if (runas_user == NULL)
      {
        runas_user = (char *)malloc(strlen(token) + 1);
        strcpy(runas_user, token);
      }

      if (token[ch] != 0)
      {
        // gettoken(in,token,128);
        if (runas_group == 0)
        {
          runas_group = (char *)malloc(strlen(token + ch) + 1);
          strcpy(runas_group, token + ch);
        }
      }
#endif
    }
      else
    if (strcasecmp(token, "jpeg_quality") == 0)
    {
      gettoken(in, token, sizeof(token));
      config->jpeg_quality = atoi(token);
    }
      else
    if (strcasecmp(token, "frame_rate") == 0)
    {
      gettoken(in, token, sizeof(token));
      config->frame_rate = atoi(token);
    }
      else
    if (strcasecmp(token, "alias") == 0)
    {
      parse_alias(in);
    }
      else
    if (strcasecmp(token, "cgi_handler") == 0)
    {
#ifdef ENABLE_CGI
      parse_handler(in);
#else
      printf("CGI support was not compiled in.\n");
      skip_block(in);
#endif
    }
      else
    if (strcasecmp(token, "plugin") == 0)
    {
#ifdef ENABLE_PLUGINS
      parse_plugin(in);
#else
      printf("Plugin support was not compiled in.\n");
      skip_block(in);
#endif
    }
      else
    if (strcasecmp(token, "capture") == 0)
    {
#ifdef ENABLE_CAPTURE
      parse_capture(in);
#else
      printf("Capture support was not compiled in.\n");
      skip_block(in);
#endif
    }
      else
    {
      printf("Config file syntax error: %s\n", token);
      break;
    }
  }

  fclose(in);

  if (username[0] != 0 || password[0] != 0)
  {
    strcat(username, ":");
    strcat(username, password);

    base64_encode(config->user_pass_64, username);
  }
}

