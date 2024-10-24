/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WINDOWS
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#endif
#include <errno.h>
#include <time.h>
#include <string.h>
#ifdef WINDOWS
#include <windows.h>
#include <process.h>
#include <winsock.h>
#endif

#ifdef JPEG_LIB
#include "jpeg_compress.h"
#endif

#ifdef ENABLE_CAPTURE
#include "capture.h"
#endif

#include "alias.h"
#include "avi_play.h"
#include "cgi_handler.h"
#include "config.h"
#include "file_io.h"
#include "general.h"
#include "http_headers.h"
#include "mime_types.h"
#include "network_io.h"
#include "plugin.h"
#include "server.h"
#include "user.h"
#include "version.h"
#include "video.h"

int sockfd;
int video_count = 0;
char *runas_user = NULL;
char *runas_group = NULL;
User **users;
User nulluser;
Video video[100];
int debug;
uint32_t uptime;
uint32_t server_flags;
Alias *alias;
#ifdef ENABLE_CGI
CgiHandler *cgi_handler;
#endif
#ifdef ENABLE_PLUGINS
Plugin *plugin;
#endif

typedef struct ThreadContext
{
  Config *config;
  int thread_num;
} ThreadContext;

void server_thread(ThreadContext *thread_context)
{
  int t = 0, r, trim;
  int id = 0;
  int msock = 0;
  fd_set readset;
  struct timeval tv;
  char *out_buffer;
  int gc_time, thread_num;
  int dirty_buffer;
  char *command, *param;

  Config *config = thread_context->config;
  thread_num     = thread_context->thread_num;
  gc_time = time(NULL);

  while (1)
  {
    if (time(NULL) - gc_time > GC_TIME)
    {
      for (r = config->minconn + thread_num; r < config->maxconn; r = r + MAX_USER_THREADS)
      {
        if (users[r]->inuse == 0 && users[r]->idletime != -1)
        {
#ifdef DEBUG
          if (debug == 1) { printf("Deallocating line %d\n",r); }
#endif
          if (time(NULL) - users[r]->idletime>GC_TIME)
          {
            free(users[r]);
            users[r] = &nulluser;
          }
        }
      }

      if (config->max_idle_time > 0)
      {
        for (r = thread_num; r < config->maxconn; r = r + MAX_USER_THREADS)
        {
          if (users[r]->inuse != 0 &&
              time(NULL) - users[r]->idletime > config->max_idle_time)
          {
            user_disconnect(users[r]);
          }
        }
      }

      gc_time = time(NULL);
    }

    FD_ZERO(&readset);
    msock = 0;
    dirty_buffer = 0;

    for (r = thread_num; r < config->maxconn; r = r + MAX_USER_THREADS)
    {
      if (users[r]->inuse == 1)
      {
        FD_SET(users[r]->socketid, &readset);

        if (msock < users[r]->socketid) { msock = users[r]->socketid; }
        if (users[r]->in_ptr < users[r]->in_len ||
            users[r]->state == STATE_SEND_FILE)
        {
          dirty_buffer = 1;
        }
      }
    }

    if (dirty_buffer == 0)
    {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
    }
      else
    {
      tv.tv_sec = 0;
      tv.tv_usec = 1;
    }

#ifdef WINDOWS
    if (msock == 0)
    {
      _sleep(1000);
      continue;
    }
#endif

    if ((t = select(msock + 1, &readset, NULL, NULL, &tv)) == -1)
    {
#ifdef WINDOWS
      if (WSAGetLastError() != WSANOTINITIALISED) { printf("yes %d\n",errno); }
      if (WSAGetLastError() != WSAEINTR)
#else
      if (errno!=EINTR)
#endif
      {
#ifdef DEBUG
        if (debug == 1) { printf("not EINTR %d\n",thread_num); }
#endif
      }
         else
      {
#ifdef DEBUG
        if (debug == 1) { printf("Problem with select\n"); }
#endif
        continue;
      }
    }

    for (id = thread_num; id < config->maxconn; id = id + MAX_USER_THREADS)
    {
      errno = 0;

      if (users[id] == 0) { continue; }
      if (users[id]->inuse != 1) { continue; }

#if 0
printf("Checking line %d     %d\n", id, users[id]->in_ptr);
printf("state=%d  need_header=%d (%d)\n",
  users[id]->state,
  users[id]->need_header,id);
#endif

      if (users[id]->state == STATE_SEND_FILE)
      {
        /* Do nothing */
      }
        else
      if (users[id]->in_ptr < users[id]->in_len ||
          FD_ISSET(users[id]->socketid, &readset))
      {
        r = buffered_read(id);
        if (r < 0) { user_disconnect(users[id]); continue; }
        if (r == 0) { continue; }
        users[id]->buffer_ptr = 0;

        if (config->user_pass_64[0] != 0)
        {
          if (strncmp(users[id]->out_buffer, "Authorization: Basic ", sizeof("Authorization: Basic ") - 1) == 0)
          {
            if ((users[id]->flags & 2) == 0 &&
                strcmp(config->user_pass_64, users[id]->out_buffer + sizeof("Authorization: Basic ") - 1) == 0)
            {
              users[id]->flags |= 1;
            }
            users[id]->flags |= 2;
          }
        }

        if (r == 2) { users[id]->state=STATE_SEND_FILE; }
      }
        else
      {
        continue;
      }

      if (users[id]->state == STATE_SEND_FILE)
      {
        if (config->user_pass_64[0] != 0)
        {
          if ((users[id]->flags & 1) != 1)
          {
            send_401(id);
            continue;
          }
        }

#ifdef ENABLE_PLUGINS
        if (users[id]->video_num == -3)  // PLUGIN
        {
           if (users[id]->method == METHOD_GET)
           {
             send_header_plugin(id);
             if (users[id]->plugin->get(users[id]->socketid, users[id]->querystring) != 0)
             {
               user_disconnct(users[id]);
               continue;
             }
           }
             else
           if (users[id]->method == METHOD_POST)
           {
             // This is totally fuckered.. need to give content length.
             send_header_plugin(id);
             if (users[id]->plugin->post(users[id]->socketid, users[id]->querystring, 0) != 0)
             {
               user_disconnct(user[id]);
               continue;
             }
           }

           users[id]->state = STATE_IDLE;
           users[id]->plugin = 0;

           continue;
        }
          else
#endif
        if (users[id]->video_num == -2)  // FILE OR CGI
        {
          send_file(id);
          continue;
        }
          else
        if (users[id]->video_num >= video_count)
        {
          send_error(id,"404 Not Found", FOUR_OH_FOUR, sizeof(FOUR_OH_FOUR));
          continue;
        }
          else
        if (users[id]->video_num >= 0)
        {

#ifdef ENABLE_CAPTURE
          if (video[users[id]->video_num].capture_info != 0)
          {
            send_capture_frame(id);
            continue;
          }
#endif

          if (users[id]->in == -1)
          {
#ifndef WINDOWS
            users[id]->in = open(video[users[id]->video_num].filename, O_RDONLY);
#else
            users[id]->in = open(video[users[id]->video_num].filename, O_RDONLY|_O_BINARY);
#endif

            if (users[id]->in == -1)
            {
              const int length = sizeof(FOUR_OH_FOUR);
#ifdef ENABLE_CGI
              if ((users[id]->mime_type & MIME_IS_CGI) == 0)
              {
                send_error(id, "404 Not Found", FOUR_OH_FOUR, length);
              }
                else
              {
                send_error(id, "400 Bad Request", FOUR_OH_OH, length);
              }
#else
              send_error(id, "404 Not Found", FOUR_OH_FOUR, length);
#endif
              continue;
            }
          }

          if (users[id]->need_header != NEED_HEADER_NO)
          {
            users[id]->idletime = time(NULL);
            r = avi_play_calc_frame(users[id]);

            if (r == users[id]->last_frame) { continue; }

            if (abs(r-users[id]->last_frame) <
                (video[users[id]->video_num].fps / users[id]->frame_rate))
            {
              continue;
            }

#ifdef DEBUG
if (debug == 1)
{
  printf("Frame %d/%d  camera=%d\n",
    r, video[users[id]->video_num].total_frames, users[id]->video_num);
}
#endif
            users[id]->last_frame = r;
          }

          send_file(id);

          continue;
        }
          else
        {
          //FIXME - why?
          continue;
        }
      }

      // if (users[id]->video_num >= 0) continue;
      // if (users[id]->video_num != -1) continue;
      if (users[id]->state != STATE_IDLE) continue;

      out_buffer = users[id]->out_buffer;

      trim = 0;

      while (out_buffer[trim] == ' ') { trim++; }

      users[id]->idletime = time(NULL);

#ifdef DEBUG
      if (debug == 1)
      {
        printf("Read in: %d bytes on thread %d.\n",
          (int)strlen(out_buffer), thread_num);
        printf("%d typed: %s\n", id, out_buffer);
        fflush(stdout);
      }
#endif

      r = 0;
      while (out_buffer[r] == ' ' && out_buffer[r] != 0) { r++; }

      command = out_buffer + r;

      r = 0;
      while (command[r] != ' '  && command[r] != '\r' &&
             command[r] != '\n' && command[r] != 0)
      {
        r++;
      }

      command[r++] = 0;

#ifdef DEBUG
      if (debug == 1)
      {
        printf("command=%s\n", command);
      }
#endif

      param = command + r;
      r = strlen(param) - 1;

      while (param[r] == ' ' || param[r] == '\r' || param[r] == '\n')
      {
        param[r--] = 0;
      }

      if (strcasecmp(command, "get") == 0)
      {
        users[id]->state = STATE_HEADERS;
        users[id]->method = METHOD_GET;

        r = conv_num(param + 1);

        if (r != users[id]->video_num && users[id]->in != -1)
        {
          file_close(users[id]);
        }

        users[id]->video_num = r;
        users[id]->need_header = NEED_HEADER_YES;
        users[id]->request_type = REQUEST_SINGLE;
        users[id]->last_frame = -1;
        users[id]->flags = 0;
        users[id]->frame_rate = config->frame_rate;
#ifdef ENABLE_CAPTURE
        users[id]->jpeg_quality = jpeg_quality;
#endif

        if (users[id]->video_num == -1)
        {
          r = 1;

          while (param[r] != ' ' && param[r] != 0) { r++; }

          param[r] = 0;
          users[id]->video_num = file_open(users[id], config, param);

#ifdef DEBUG
if (debug == 1)
{
  printf("video_num=%d\n", users[id]->video_num);
}
#endif

        }
      }
#ifdef ENABLE_CGI
        else
      if (strcasecmp(command, "post") == 0)
      {
         // complete me
         // this maybe should go above in the GET section
      }
#endif
        else
      {
        // Unknown command.
        user_disconnect(users[id]);
      }
    }
  }
}

int server_run(Config *config)
{
  struct sockaddr_in serv_addr;
  int r;
  int newsockfd;
  socklen_t clilen;
  struct sockaddr_in cli_addr;
  ThreadContext thread_context[MAX_USER_THREADS];
#ifndef WINDOWS
  pthread_t pid;
#endif

  uptime = time(NULL);

#ifndef WINDOWS
  if (debug == 0)
  {
    r = fork();

    if (r == -1)
    {
      perror("Error: Could not fork\n"); exit(3);
    }
      else
    if (r == 0)
    {
      close(STDIN_FILENO);
      close(STDERR_FILENO);

      if (setsid() == -1) { exit(4); }
    }
      else
    {
      return 0;
    }
  }
#endif

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("Can't open socket.\n");
    return -1;
  }

  memset((char *)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(config->port);

  set_socket_options(sockfd);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("server can't bind.\n");
    return -1;
  }

  listen(sockfd, 5);
#ifndef WINDOWS
  set_signals();
  config_set_runas(config);
#endif

  printf("\n" VERSION "\n" COPYRIGHT "\n\n");

  printf("Started on port: %d\n", config->port);
  fflush(stdout);

  if (debug == 0) { close(STDOUT_FILENO); }

  nulluser.inuse = 0;
  nulluser.idletime = -1;
  users = malloc(sizeof(User *) * config->maxconn);

  for (r = 0; r < config->maxconn; r++)
  {
    users[r] = &nulluser;
  }

  for (r = 0; r < config->minconn; r++)
  {
    users[r] = malloc(sizeof(User));
    users[r]->inuse = 0;
  }

  for (r = 0; r < MAX_USER_THREADS; r++)
  {
    thread_context[r].config = config;
    thread_context[r].thread_num = r;

#ifndef WINDOWS
    pthread_create(&pid, NULL, (void *)server_thread, &thread_context[r]);
#else
    _beginthread((void *)server_thread, 0, &thread_context[r]);
#endif
  }

  clilen = sizeof(cli_addr);

  while (1)
  {
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    if (newsockfd < 0)
    {
#ifdef DEBUG
      if (debug == 1) { printf("server accept error.\n"); }
#endif
    }
      else
    {
#ifdef DEBUG
      if (debug == 1)
      {
        printf("New socket accepted\n");
        fflush(stdout);
      }
#endif

      user_connect(config, newsockfd, &cli_addr);
    }
  }
}

