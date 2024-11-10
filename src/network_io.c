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
#ifndef WINDOWS
#include <sys/types.h>
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
#include <winsock.h>
#endif

#include "file_io.h"
#include "general.h"
#include "globals.h"
#include "http_headers.h"
#include "functions.h"
#include "mime_types.h"
#include "network_io.h"
#include "user.h"

int send_data(int socketid, const char *message, int message_len)
{
  struct timeval tv;
  fd_set write_set;
  int t;

  while (1)
  {
    FD_ZERO(&write_set);
    FD_SET(socketid, &write_set);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if ((t = select(socketid + 1, NULL, &write_set, NULL, &tv)) == -1)
    {
      if (errno == EINTR) { continue; }

      printf("Problem with select\n");
      return -1;
    }
      else
    {
      if (t == 0) { return -1; }

      t = send(socketid, message, message_len, 0);

      return t;
    }
  }
}

int send_file(int id)
{
  char temp_string[BIGGEST_FILE_CHUNK];
  int t, k, r, c;

  if (users[id]->in == -1)
  {
    user_disconnect(users[id]);
    return -1;
  }

  if (users[id]->need_header == NEED_HEADER_YES)
  {
    if (users[id]->request_type == REQUEST_SINGLE)
    {
#ifdef ENABLE_CGI
      if ((users[id]->mime_type & MIME_IS_CGI) != 0)
      {
        send_header_cgi(id);
      }
        else
#endif
      {
        send_header(id);
      }
    }
      else
    if (users[id]->request_type == REQUEST_MULTIPART)
    {
      send_header_multipart(id);
    }
      else
    if (users[id]->request_type == REQUEST_MULTIPART2)
    {
      sprintf(temp_string,
        "\r\n--myboundary"
        "\r\nContent-Type: image/jpeg\r\nContent-Length: %d"
        "\r\n\r\n",
        users[id]->content_length);

      message(id, temp_string);
    }

    users[id]->need_header = NEED_HEADER_NO;
  }

  if (users[id]->in == -1) { return -1; }

/*
if (debug==1)
{ printf("sending bytes... %ld left\n",users[id]->content_length); }
*/

#ifdef NO_INTERLACED_FILE
  while (users[id]->content_length > 0)
#else
  for (c = 0; c < CHUNKS_PER_SEND; c++)
#endif
  {
    if (users[id]->content_length == 0) { break; }

    // This 1024 thing needs work.
#ifdef WITH_MMAP
    if (users[id]->video_num >= 0)
    {
      if (users[id]->content_length > BIGGEST_FILE_CHUNK)
      {
        r = BIGGEST_FILE_CHUNK;
      }
        else
      {
        r = users[id]->content_length;
      }
    }
      else
#endif
    {
/*
      if (users[id]->content_length > BIGGEST_FILE_CHUNK)
      {
        r=fread(temp_string, 1, BIGGEST_FILE_CHUNK, users[id]->in);
      }
        else
      {
        r = fread(temp_string, 1, users[id]->content_length, users[id]->in);
      }
*/

      if (users[id]->content_length > BIGGEST_FILE_CHUNK)
      {
        r=read(users[id]->in, temp_string, BIGGEST_FILE_CHUNK);
      }
        else
      {
        r = read(users[id]->in, temp_string, users[id]->content_length);
      }
    }

    users[id]->content_length -= r;

#ifdef ENABLE_CGI
    if (r == 0 && (users[id]->mime_type & MIME_IS_CGI) != 0)
    {
      users[id]->content_length = 0;
      user_disconnect(users[id]);
      return 0;
      // break;
    }
#endif

    t = 0;
    while (t < r)
    {
#ifdef WITH_MMAP
      if (users[id]->video_num >= 0)
      {
        k = send_data(users[id]->socketid, video[users[id]->video_num].mem + users[id]->offset + t, r - t);
      }
        else
#endif
      {
        k = send_data(users[id]->socketid, temp_string + t, r - t);
      }

      if (k == -1)
      {
        user_disconnect(users[id]);
        return -1;
      }

      t = t + k;
    }

#ifdef WITH_MMAP
  users[id]->offset += r;
#endif

  }

  if (users[id]->content_length == 0)
  {
    if (users[id]->video_num == -2)
    {
      if (users[id]->in != -1)
      {
        file_close(users[id]);
      }

      users[id]->video_num = -1;
    }

    if (users[id]->request_type == REQUEST_SINGLE)
    {
      users[id]->state = STATE_IDLE;
    }
      else
    {
      users[id]->need_header = NEED_HEADER_YES;
    }
  }

  return 0;
}

#ifdef ENABLE_CAPTURE
int send_capture_frame(int id)
{
  if (users[id]->need_header == NEED_HEADER_YES)
  {
    users[id]->jpeg_ptr = 0;
    users[id]->mime_type = 4;

    users[id]->content_length =
      capture_image(video[users[id]->video_num].capture_info, id);

#ifdef DEBUG
if (debug == 1) { printf("content-length: %d\n", users[id]->content_length); }
#endif

    if (users[id]->content_length < 0) { users[id]->content_length = 0; }

#if 0
    if (users[id]->jpeg_len == 0)
    {
// FIXME this should be dynamic instead of wasting memory.. easy to fix
//       also need a way to deallocate this crap

      // users[id]->jpeg_len = 20480;
      users[id]->jpeg_len = 128000;
      users[id]->jpeg = (uint8_t *)malloc(users[id]->jpeg_len);
    }
#endif

#if 0
    users[id]->content_length = jpeg_compress(
      cap_buffer,
      video[users[id]->video_num].capture_info->buffer_len,
      users[id]->jpeg,
      users[id]->jpeg_len,
      video[users[id]->video_num].capture_info->width,
      video[users[id]->video_num].capture_info->height,
      3,
      users[id]->jpeg_quality);
#endif

    switch (users[id]->request_type)
    {
      case REQUEST_SINGLE:
      {
        send_header(id);
        break;
      }
      case REQUEST_MULTIPART:
      {
        send_header_multipart(id);
        break;
      }
      case REQUEST_MULTIPART2:
      {
        char temp_string[96];

        sprintf(temp_string,
          "\r\n--myboundary"
          "\r\nContent-Type: image/jpeg"
          "\r\nContent-Length: %d\r\n\r\n",
          users[id]->content_length);

        message(id, temp_string);
        break;
      }
    }

    users[id]->need_header = NEED_HEADER_NO;
  }

//FIXME: REMOVE
//printf("send_capture_frame() %d/%d %d chunks=%d\n",
//  users[id]->jpeg_ptr,
//  users[id]->content_length, id,
//  CHUNKS_PER_SEND);

#ifdef NO_INTERLACED_FILE
  while (users[id]->jpeg_ptr < users[id]->content_length)
#else
  int c;

  for (c = 0; c < CHUNKS_PER_SEND; c++)
#endif
  {
    // This BIGGEST_FILE_CHUNK thing needs work.
    int chunk_length = users[id]->content_length - users[id]->jpeg_ptr;

#if 0
    if (chunk_length > BIGGEST_FILE_CHUNK)
    {
      chunk_length = users[id]->jpeg_ptr + BIGGEST_FILE_CHUNK;
    }
#endif

    const int socketid = users[id]->socketid;

    while (users[id]->jpeg_ptr < chunk_length)
    {
//printf("Preparing to send %d bytes\n", chunk_length - users[id]->jpeg_ptr);
      int length = chunk_length - users[id]->jpeg_ptr;
      if (length > 4096) { length = 4096; }

      int last_length = send_data(
        socketid,
        (char *)(users[id]->jpeg + users[id]->jpeg_ptr),
        //chunk_length - users[id]->jpeg_ptr);
        length);

//printf("%d %d %d\n", users[id]->jpeg_ptr, chunk_length, last_length);
      if (last_length == -1)
      {
        user_disconnect(users[id]);
        return -1;
      }

      users[id]->jpeg_ptr += last_length;
    }

    if (users[id]->jpeg_ptr == users[id]->content_length) { break; }
  }

  if (users[id]->jpeg_ptr == users[id]->content_length)
  {
    if (users[id]->request_type == REQUEST_SINGLE)
    {
      users[id]->state = STATE_IDLE;
    }
      else
    {
      users[id]->need_header = NEED_HEADER_YES;
    }
  }

  return 0;
}
#endif

int buffered_read(int id)
{
  int r;

  while (1)
  {
    errno = 0;

    if (users[id]->in_ptr >= users[id]->in_len)
    {
      users[id]->in_len = recv(users[id]->socketid, (char *)users[id]->in_buffer, BUFFER_SIZE - 2, 0);
      users[id]->in_ptr = 0;

      if (users[id]->in_len == 0) { return -1; }
      if (users[id]->in_len < 0)
      {
#ifdef DEBUG
        if (debug == 1)
        {
          printf("Lost Connection (NULL): %d\n",id);
          fflush(stdout);
        }
#endif

        users[id]->buffer_ptr = 0;
        users[id]->in_len = 0;
        return -1;
      }
    }

    for (r = users[id]->in_ptr; r < users[id]->in_len; r++)
    {
      if (users[id]->in_buffer[r] == '\r') { continue; }

      if (users[id]->in_buffer[r] == '\n')
      {
        users[id]->out_buffer[users[id]->buffer_ptr] = 0;
        users[id]->in_ptr = r + 1;
        return (users[id]->buffer_ptr == 0) ? 2 : 1;
      }

      if (users[id]->in_buffer[r] == 127)
      {
        if (users[id]->buffer_ptr > 0) { users[id]->buffer_ptr--; }
      }
        else
      {
        if (users[id]->in_buffer[r] >= ' ' && users[id]->in_buffer[r] < 254)
        {
          users[id]->out_buffer[users[id]->buffer_ptr++] =
            users[id]->in_buffer[r];
        }
      }

      if (users[id]->buffer_ptr == BUFFER_SIZE - 1)
      {
        users[id]->out_buffer[users[id]->buffer_ptr] = 0;
        users[id]->in_ptr = r + 1;
        return 1;
      }
    }

    users[id]->in_ptr = r + 1;
  }
}

int set_socket_options(int sockfd)
{
  int sopt = 1;
  struct linger mylinger;
  mylinger.l_onoff = 0;
  mylinger.l_linger = 1;

#ifndef WINDOWS
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sopt, sizeof(sopt)))
  {
    printf("socket options REUSERADDR can't be set.\n");
    return -1;
  }

  if (setsockopt(sockfd, SOL_SOCKET,SO_LINGER, &mylinger, sizeof(struct linger)))
  {
    printf("socket options LINGER can't be set.\n");
    return -1;
  }
#else
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sopt, sizeof(sopt)))
  {
    printf("socket options REUSERADDR can't be set.\n");
    return -1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char *)&mylinger, sizeof(struct linger)))
  {
    printf("socket options LINGER can't be set.\n");
    return -1;
  }

  timeBeginPeriod(5);
#endif

  return 0;
}

int set_nonblocking(int socketid)
{
#ifndef WINDOWS
  fcntl(socketid, F_SETFL, O_NONBLOCK);
#else
  u_long arg = 1L;
  ioctlsocket(socketid, FIONBIO, (u_long FAR *)&arg);
#endif

  return 0;
}

