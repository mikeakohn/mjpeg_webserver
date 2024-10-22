/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#ifndef AVI_H
#define AVI_H

struct avi_header_t
{
  int time_delay;
  int data_rate;
  int reserved;
  int flags;
  int number_of_frames;
  int initial_frames;
  int data_streams;
  int buffer_size;
  int width;
  int height;
  int time_scale;
  int playback_data_rate;
  int start_time;
  int data_length;
};

struct stream_header_t
{
  char data_type[5];
  char codec[5];
  int flags;
  int priority;
  int initial_frames;
  int time_scale;
  int data_rate;
  int start_time;
  int data_length;
  int buffer_size;
  int quality;
  int sample_size;
};

struct stream_format_t
{
  int header_size;
  int image_width;
  int image_height;
  int number_of_planes;
  int bits_per_pixel;
  int compression_type;
  int image_size_in_bytes;
  int x_pels_per_meter;
  int y_pels_per_meter;
  int colors_used;
  int colors_important;
  int *palette;
};

struct index_entry_t
{
  char ckid[5];
  int flags;
  int offset;
  int length;
};

#endif

