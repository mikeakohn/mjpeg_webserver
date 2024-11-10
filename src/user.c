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
#include <netdb.h>
#include <time.h>

#include "config.h"
#include "file_io.h"
#include "general.h"
#include "globals.h"
#include "network_io.h"
#include "user.h"

User **users;

void user_init(User *user, int id, int socketid)
{
  user->inuse = 200;
  user->id = id;
  user->socketid = socketid;

  user->in_len = 0;
  user->in_ptr = 0;
  user->buffer_ptr = 0;
  user->logontime = time(NULL);
  user->idletime = time(NULL);
  user->curr_frame = -1;
  user->video_num = -1;
  user->in = -1;
  user->pin = NULL;
  user->inuse = 1;
  user->state = STATE_IDLE;
#ifdef ENABLE_PLUGINS
  user->plugin = NULL;
#endif
}

void user_destroy(User *user)
{
}

int user_connect(Config *config, int socketid, struct sockaddr_in *cli_addr)
{
  int k, r, id;
  // struct hostent *my_hostent = NULL;
  uint8_t *address = NULL;
  const char *fullmessage = "mjpeg_webserver is full\r\n";

#if 0
  if ((server_flags & 2) == 2)
  {
    my_hostent = gethostbyaddr((char *)&(cli_addr->sin_addr.s_addr), 4, AF_INET);
  }
#endif

  for (r = 0; r < config->maxconn; r++)
  {
    if (users[r]->inuse == 0) { break; }
  }

  if (r >= config->maxconn)
  {
#ifdef DEBUG
    if (debug == 1) { printf("mjpeg_webserver is full\n"); }
#endif
    send_data(socketid, fullmessage, strlen(fullmessage));
    socketdie(socketid);
    return -1;
  }

  if (users[r]->idletime == -1)
  {
#ifdef DEBUG
    if (debug == 1) { printf("Allocing new user %d\n",r); }
#endif
    users[r] = malloc(sizeof(User));
  }

#ifdef DEBUG
  if (debug == 1)
  {
    printf("searched for open User: found %d\n",r);
    fflush(stdout);
  }
#endif

/*
  if (my_hostent != NULL && (server_flags & 2) == 2)
  { safestrncpy(users[r]->location, my_hostent->h_name, 61); }
    else
*/
  {
    address = (uint8_t*)&cli_addr->sin_addr.s_addr;

    sprintf((char *)users[r]->location, "%d.%d.%d.%d",
      address[0],
      address[1],
      address[2],
      address[3]);
  }

#ifdef DEBUG
  if (debug == 1)
  {
    printf("Connected from location: %s\n", users[r]->location);
    fflush(stdout);
  }
#endif

  k = 0;

  if ((server_flags & 8) == 8)
  {
    for (k = 0; k < config->maxconn; k++)
    {
      if (users[k]->inuse != 1) { continue; }

      if (strcmp((char *)users[r]->location,(char *)users[k]->location) == 0)
      {
        k = -1;
        break;
      }
    }
  }

  id = r;

  user_init(users[id], id, socketid);

#if 0
  users[id]->socketfd = fdopen(socketid, "rb+");

  if (users[id]->socketfd == 0) { printf("Problem opening file desc.\n\n"); }
#endif

  set_nonblocking(users[id]->socketid);

#if 0
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVLOWAT, &sopt, sizeof(sopt)))
  {
    printf("socket options SO_RCVLOWAT can't be set.\n");
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDLOWAT, &sopt,sizeof(sopt)))
  {
    printf("socket options SO_SNDLOWAT can't be set.\n");
  }
#endif

#ifdef DEBUG
if (debug == 1)
{
  printf("id=%d ready to go\n", id);
  fflush(stdout);
}
#endif

  return 0;
}

void user_disconnect(User *user)
{
#ifdef DEBUG
  if (debug == 1)
  {
    printf("Getting ready to close socket.\n");

    if (user->inuse != 4)
    {
      printf("Socket Close Return value: %d:\n", socketdie(user->socketid));
    }
  }
#endif

  if (user->inuse != 4) { socketdie(user->socketid); }

  user->socketid = -1;

#ifdef DEBUG
  if (debug == 1) { printf("Connection dead: %d\n",user->id); }
  fflush(stdout);
#endif

  if (user->in != -1)
  {
    // fclose(users[id]->in);
    file_close(user);
  }

  user->inuse = 0;
  user->idletime = time(NULL);
}

