/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "alias.h"
#include "cgi_handler.h"
#include "config.h"
#include "file_io.h"
#include "globals.h"
#include "mime_types.h"
#include "plugin.h"
#include "user.h"
#include "url_utils.h"

int file_open(User *user, Config *config, char *filename)
{
  Alias *curr_alias;
  struct stat file_stat;
  char *querystring = NULL;
#ifdef ENABLE_CGI
  char handler_file[2048];
  CgiHandler *curr_handler;
  int t;
#endif
#ifdef ENABLE_PLUGINS
  Plugin *curr_plugin;
#endif

  if (filename[0] == 0) { return VIDEO_NUM_404; }

#ifdef DEBUG
if (debug == 1)
{
  printf("filename: %s\n", filename);
}
#endif

  curr_alias = alias;

  while (curr_alias != NULL)
  {
    if (strncmp(filename, curr_alias->url, curr_alias->url_len) == 0)
    {
      user->video_num = 0;

      if (curr_alias->type != 0)
      {
        user->request_type = REQUEST_MULTIPART;
      }

      parse_querystring(user, filename, curr_alias);

      return user->video_num;
    }

    curr_alias = curr_alias->next_alias;
  }

#ifdef ENABLE_PLUGINS
  curr_plugin = plugin;

  while (curr_plugin != NULL)
  {
    if (strncmp(filename, curr_plugin->alias, curr_plugin->alias_len) == 0)
    {
      // send_header_plugin(id);

      // Here I am, and you're a querystring.
      querystring = filename + curr_plugin->alias_len;
      if (querystring[0]=='?') { querystring=querystring + 1; }

      user->plugin = curr_plugin;
      snprintf(user->querystring, QUERY_STRING_SIZE, querystring);

#if 0
      if (curr_plugin->get(user->socketid, querystring) != 0)
      {
        kill_user(id);
      }
#endif

      return -3;
    }

    curr_plugin = curr_plugin->next_plugin;
  }
#endif

  querystring = get_querystring(filename);

  if (check_valid_file(filename) == -1)
  {
    user->video_num = -1;
    return VIDEO_NUM_404;
  }

  const char *name = config->index_file == NULL ?
    "index.html" : config->index_file;

  int length = strlen(filename) + strlen(config->htdocs_dir) + strlen(name) + 1;

  if (length >= 2048)
  {
    return VIDEO_NUM_404;
  }

  char full_file[length];

  if (filename[0] == '/' && filename[1] == 0)
  {
    snprintf(full_file, length, "%s/%s", config->htdocs_dir, name);
    user->mime_type = MIME_TYPE_HTML;
  }
    else
  {
    url_decode((uint8_t *)filename);

    if (filename[0] != '/')
    {
      snprintf(full_file, sizeof(full_file), "%s/%s", config->htdocs_dir, filename);
    }
      else
    {
      sprintf(full_file, "%s%s", config->htdocs_dir, filename);
    }

    user->mime_type = get_mime_type_code(filename);

    if (user->in != -1) { file_close(user); }

#ifdef ENABLE_CGI
    setenv("QUERY_STRING", querystring, 1);

    if ((user->mime_type & MIME_IS_CGI) != 0)
    {
      t = MIME_IS_CGI;
      curr_handler = cgi_handler;

      while(t != user->mime_type)
      {
        curr_handler = curr_handler->next_handler;
        t++;
      }

      if (curr_handler->application != NULL)
      {
        snprintf(handler_file, sizeof(handler_file), "%s '%s'",
          curr_handler->application, full_file);

        user->pin = popen(handler_file, "r");

        if (user->pin != NULL)
        {
          user->in = fileno(user->pin);
        }
      }
        else
      {
        snprintf(handler_file, sizeof(handler_file), "'%s'", full_file);

        user->pin = popen(handler_file, "r");

        if (user->pin != NULL)
        {
          user->in = fileno(user->pin);
        }
      }

      // unsetenv might not be needed actually.
      unsetenv("QUERY_STRING");

      if (user->in == -1)
      {
        return VIDEO_NUM_400;
      }

      return -2;
    }
#endif
  }

#ifdef DEBUG
if (debug == 1)
{
  printf("mime-type: %s\n", mime_types[user->mime_type]);
  printf("opening: %s\n", full_file);
}
#endif

  // REVIEW: Is fopen() good enough?
  // user->in = fopen(full_file,"rb");
#ifndef WINDOWS
  user->in = open(full_file, O_RDONLY);
#else
  user->in = open(full_file, O_RDONLY | _O_BINARY);
#endif

  if (user->in == -1) { return VIDEO_NUM_404; }

  user->last_modified = time(NULL);

  // file len.
  // fseek(user->in, 0, SEEK_END);
  //lseek(user->in,0,SEEK_END);
  //user->content_length = ftell(user->in);
  // rewind(user->in);

  fstat(user->in, &file_stat);

  if ((file_stat.st_mode & S_IFMT) == S_IFDIR)
  {
    close(user->in);
    user->in = -1;
    return VIDEO_NUM_404;
  }

  user->content_length = file_stat.st_size;

  return -2;
}

int file_close(User *user)
{
  if (user->in == -1) { return 0; }

  int in = user->in;

  user->in = -1;

#ifdef ENABLE_CGI
  if ((user->mime_type & MIME_IS_CGI ) != 0)
  {
    return pclose(user->pin);
  }
#endif

  return close(in);
}

int read_int32(FILE *in)
{
  int c;

  c = getc(in);
  c = c | (getc(in) << 8);
  c = c | (getc(in) << 16);
  c = c | (getc(in) << 24);

  return c;
}

int read_int16(FILE *in)
{
  int c;

  c = getc(in);
  c = c | (getc(in) << 8);

  return c;
}

int read_chars(FILE *in, char *s, int count)
{
  int t;

  for (t = 0; t < count; t++)
  {
    s[t] = getc(in);
  }

  s[t] = 0;

  return 0;
}

int read_bytes(FILE *in, char *s, int count)
{
  int t;

  for (t = 0; t < count; t++)
  {
    s[t] = getc(in);
  }

  return 0;
}

