/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef PLUGIN_HH
#define PLUGIN_HH

#ifdef ENABLE_PLUGINS

typedef int (*mjpeg_webserver_plugin_init_t)();
typedef int (*mjpeg_webserver_plugin_get_t)(int,char*);
typedef int (*mjpeg_webserver_plugin_post_t)(int,char*,int);
#ifdef WINDOWS
#define dlsym GetProcAddress
#define dlopen(libname,x) LoadLibrary(libname)
#define dlclose(libhandle) FreeLibrary(libhandle)
#define socklen_t int
#endif

typedef struct Plugin
{
  struct Plugin *next_plugin;
  char *alias;
  int alias_len;
#ifndef WINDOWS
  void *dlhandle;
  mjpeg_webserver_plugin_get_t  get;
  mjpeg_webserver_plugin_post_t post;
/*
  void mjpeg_webserver_plugin_init();
  int mjpeg_webserver_plugin_get(int fd_out, char *querystring);
  int mjpeg_webserver_plugin_post(int fd_out, char *querystring, int fd_in, int content_length);
*/
#else
  HMODULE dlhandle;
  mjpeg_webserver_plugin_init_t init;
  mjpeg_webserver_plugin_get_t  get;
  mjpeg_webserver_plugin_post_t post;
#endif
} Plugin;

extern Plugin *plugin;

#endif

#endif

