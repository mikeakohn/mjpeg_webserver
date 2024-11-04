/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "avi.h"
#include "file_io.h"
#include "globals.h"
#include "video.h"

int skip_chunk(FILE *in)
{
  char chunk_id[4];
  int chunk_size;
  int end_of_chunk;

  read_bytes(in, chunk_id, 4);
  chunk_size = read_int32(in);

#ifdef DEBUG
  printf("Uknown Chunk at %d\n", (int)ftell(in));
  printf("-------------------------------\n");
  printf("           chunk_id: %.4s\n", chunk_id);
  printf("         chunk_size: %d\n", chunk_size);
  printf("\n");
#endif

  end_of_chunk = ftell(in) + chunk_size;

  if ((end_of_chunk % 4) != 0)
  {
    end_of_chunk = end_of_chunk + (4 - (end_of_chunk % 4));
  }

  fseek(in, end_of_chunk, SEEK_SET);

  return 0;
}

int parse_idx1(FILE *in, int chunk_len, Video *video, int movi_ptr)
{
  struct index_entry_t index_entry;
  int ptr;
  int t, ptr_flag;

#ifdef DEBUG
  printf("IDX1\n");
  printf("-------------------------------\n");
  printf("ckid   flags           offset               length\n");
#endif

  video->total_frames = chunk_len / 16;
  video->index = malloc(sizeof(long) * (video->total_frames));
  ptr_flag = 0;
  ptr = 0;

  for (t = 0; t < chunk_len / 16; t++)
  {
    read_bytes(in, index_entry.ckid, 4);
    index_entry.flags  = read_int32(in);
    index_entry.offset = read_int32(in);
    index_entry.length = read_int32(in);

    if (t == 0)
    {
      if (index_entry.offset < 16) { ptr_flag = 1; }
    }

    if (ptr_flag == 1)
    {
      video->index[ptr++] = index_entry.offset + movi_ptr;
    }
      else
    {
      video->index[ptr++] = index_entry.offset;
    }

#if 0
#ifdef DEBUG
    printf("%.4s   0x%08x      0x%08x           0x%08x  0x%08x\n",
             index_entry.ckid,
             index_entry.flags,
             index_entry.offset,
             index_entry.length,
             (int)video->index[ptr-1]);
#endif
#endif
  }

#ifdef DEBUG
  printf("\n");
#endif

  return 0;
}

int read_avi_header(FILE *in, struct avi_header_t *avi_header)
{
  avi_header->time_delay         = read_int32(in);
  avi_header->data_rate          = read_int32(in);
  avi_header->reserved           = read_int32(in);
  avi_header->flags              = read_int32(in);
  avi_header->number_of_frames   = read_int32(in);
  avi_header->initial_frames     = read_int32(in);
  avi_header->data_streams       = read_int32(in);
  avi_header->buffer_size        = read_int32(in);
  avi_header->width              = read_int32(in);
  avi_header->height             = read_int32(in);
  avi_header->time_scale         = read_int32(in);
  avi_header->playback_data_rate = read_int32(in);
  avi_header->start_time         = read_int32(in);
  avi_header->data_length        = read_int32(in);

#ifdef DEBUG
  printf("AVI HEADER\n");
  printf("-------------------------------\n");
  printf("          TimeDelay: %d\n", avi_header->time_delay);
  printf("           DataRate: %d\n", avi_header->data_rate);
  printf("           Reserved: %d\n", avi_header->reserved);
  printf("              Flags: %d\n", avi_header->flags);
  printf("     NumberOfFrames: %d\n", avi_header->number_of_frames);
  printf("      InitialFrames: %d\n", avi_header->initial_frames);
  printf("        DataStreams: %d\n", avi_header->data_streams);
  printf("         BufferSize: %d\n", avi_header->buffer_size);
  printf("              Width: %d\n", avi_header->width);
  printf("             Height: %d\n", avi_header->height);
  printf("          TimeScale: %d\n", avi_header->time_scale);
  printf("   PlaybackDataRate: %d\n", avi_header->playback_data_rate);
  printf("          StartTime: %d\n", avi_header->start_time);
  printf("         DataLength: %d\n", avi_header->data_length);
  printf("\n");
#endif

  return 0;
}

int read_stream_header(FILE *in, struct stream_header_t *stream_header)
{
  read_bytes(in,stream_header->data_type, 4);
  read_bytes(in,stream_header->codec, 4);
  stream_header->flags          = read_int32(in);
  stream_header->priority       = read_int32(in);
  stream_header->initial_frames = read_int32(in);
  stream_header->time_scale     = read_int32(in);
  stream_header->data_rate      = read_int32(in);
  stream_header->start_time     = read_int32(in);
  stream_header->data_length    = read_int32(in);
  stream_header->buffer_size    = read_int32(in);
  stream_header->quality        = read_int32(in);
  stream_header->sample_size    = read_int32(in);

#ifdef DEBUG
  printf("STREAM HEADER\n");
  printf("-------------------------------\n");
  printf("             DataType: %.4s\n", stream_header->data_type);
  printf("                Codec: %.4s\n", stream_header->codec);
  printf("                Flags: %d\n", stream_header->flags);
  printf("             Priority: %d\n", stream_header->priority);
  printf("        InitialFrames: %d\n", stream_header->initial_frames);
  printf("            TimeScale: %d\n", stream_header->time_scale);
  printf("             DataRate: %d\n", stream_header->data_rate);
  printf("            StartTime: %d\n", stream_header->start_time);
  printf("           DataLength: %d\n", stream_header->data_length);
  printf("           BufferSize: %d\n", stream_header->buffer_size);
  printf("              Quality: %d\n", stream_header->quality);
  printf("           SampleSize: %d\n", stream_header->sample_size);
  printf("\n");
#endif

  return 0;
}

int read_stream_format(FILE *in, struct stream_format_t *stream_format)
{
  int t, r, g, b;

  stream_format->header_size         = read_int32(in);
  stream_format->image_width         = read_int32(in);
  stream_format->image_height        = read_int32(in);
  stream_format->number_of_planes    = read_int16(in);
  stream_format->bits_per_pixel      = read_int16(in);
  stream_format->compression_type    = read_int32(in);
  stream_format->image_size_in_bytes = read_int32(in);
  stream_format->x_pels_per_meter    = read_int32(in);
  stream_format->y_pels_per_meter    = read_int32(in);
  stream_format->colors_used         = read_int32(in);
  stream_format->colors_important    = read_int32(in);
  stream_format->palette = 0;

  if (stream_format->colors_important != 0)
  {
    stream_format->palette = malloc(stream_format->colors_important * sizeof(int));

    for (t = 0; t  <stream_format->colors_important; t++)
    {
      b = getc(in);
      g = getc(in);
      r = getc(in);
      stream_format->palette[t] = (r << 16) + (g << 8) + b;
    }
  }

#ifdef DEBUG
  printf("STREAM FORMAT\n");
  printf("-------------------------------\n");
  printf("        header_size: %d\n", stream_format->header_size);
  printf("        image_width: %d\n", stream_format->image_width);
  printf("       image_height: %d\n", stream_format->image_height);
  printf("   number_of_planes: %d\n", stream_format->number_of_planes);
  printf("     bits_per_pixel: %d\n", stream_format->bits_per_pixel);
  printf("   compression_type: %04x (%c%c%c%c)\n",
     stream_format->compression_type,
   ((stream_format->compression_type) & 0xff),
   ((stream_format->compression_type >> 8) & 0xff),
   ((stream_format->compression_type >> 16) & 0xff),
   ((stream_format->compression_type >> 24) & 0xff));
  printf("image_size_in_bytes: %d\n", stream_format->image_size_in_bytes);
  printf("   x_pels_per_meter: %d\n", stream_format->x_pels_per_meter);
  printf("   y_pels_per_meter: %d\n", stream_format->y_pels_per_meter);
  printf("        colors_used: %d\n", stream_format->colors_used);
  printf("   colors_important: %d\n", stream_format->colors_important);
  printf("\n");
#endif

  return 0;
}

int parse_hdrl_list(
  FILE *in,
  struct avi_header_t *avi_header,
  struct stream_header_t *stream_header,
  struct stream_format_t *stream_format)
{
  char chunk_id[4];
  int chunk_size;
  char chunk_type[4];
  int end_of_chunk;
  int next_chunk;

  read_bytes(in, chunk_id, 4);
  chunk_size = read_int32(in);
  read_bytes(in, chunk_type, 4);

#ifdef DEBUG
  printf("AVI Header Chunk LIST\n");
  printf("-------------------------------\n");
  printf("           chunk_id: %.4s\n", chunk_id);
  printf("         chunk_size: %d\n", chunk_size);
  printf("         chunk_type: %.4s\n", chunk_type);
  printf("\n");
#endif

  end_of_chunk = ftell(in) + chunk_size - 4;

  if ((end_of_chunk % 4) != 0)
  {
    end_of_chunk = end_of_chunk + (4 - (end_of_chunk % 4));
  }

  while (ftell(in) < end_of_chunk)
  {
    read_bytes(in, chunk_type, 4);
    chunk_size = read_int32(in);
    next_chunk = ftell(in) + chunk_size;

    if ((chunk_size % 4) != 0)
    {
      chunk_size = chunk_size + (4 - (chunk_size % 4));
    }

    if (strncasecmp("strh", chunk_type, 4) == 0)
    {
      read_stream_header(in, stream_header);
    }
      else
    if (strncasecmp("strf", chunk_type, 4) == 0)
    {
      read_stream_format(in, stream_format);
    }
      else
    if (strncasecmp("strd", chunk_type, 4) == 0)
    {

    }
      else
    {
#ifdef DEBUG
      printf("Unkown chunk type: %.4s\n", chunk_type);
#endif
      /* skip_chunk(in); */
    }

    fseek(in, next_chunk, SEEK_SET);
  }

  fseek(in, end_of_chunk, SEEK_SET);

  return 0;
}

int parse_hdrl(
  FILE *in,
  struct avi_header_t *avi_header,
  struct stream_header_t *stream_header,
  struct stream_format_t *stream_format)
{
  char chunk_id[4];
  int chunk_size;
  int end_of_chunk;

  read_bytes(in, chunk_id, 4);
  chunk_size = read_int32(in);

#ifdef DEBUG
  printf("AVI Header Chunk\n");
  printf("-------------------------------\n");
  printf("           chunk_id: %.4s\n", chunk_id);
  printf("         chunk_size: %d\n", chunk_size);
  printf("\n");
#endif

  end_of_chunk = ftell(in) + chunk_size;

  if ((end_of_chunk % 4) != 0)
  {
    end_of_chunk = end_of_chunk + (4 - (end_of_chunk % 4));
  }

  read_avi_header(in, avi_header);
  parse_hdrl_list(in, avi_header, stream_header, stream_format);

  return 0;
}

int parse_riff(FILE *in, Video *video)
{
  char chunk_id[4];
  int chunk_size;
  char chunk_type[4];
  long end_of_chunk, end_of_subchunk;
  struct avi_header_t avi_header;
  struct stream_header_t stream_header;
  struct stream_format_t stream_format;
  long movi_ptr = 0;
  float float_fps;

  read_bytes(in, chunk_id, 4);
  chunk_size = read_int32(in);
  read_bytes(in, chunk_type, 4);

#ifdef DEBUG
  printf("RIFF Chunk\n");
  printf("-------------------------------\n");
  printf("           chunk_id: %.4s\n", chunk_id);
  printf("         chunk_size: %d\n", chunk_size);
  printf("         chunk_type: %.4s\n", chunk_type);
  printf("\n");
#endif

  if (memcmp("RIFF", chunk_id, 4) != 0)
  {
#ifdef DEBUG
    printf("Not a RIFF file.\n");
#endif
    return -1;
  }
    else
  if (memcmp("AVI ", chunk_type, 4) != 0)
  {
#ifdef DEBUG
    printf("Not an AVI file.\n");
#endif
    return -1;
  }

  end_of_chunk = ftell(in) + chunk_size - 4;

#if 0
  if ((end_of_chunk % 4) != 0)
  {
    end_of_chunk = end_of_chunk + (4 - (end_of_chunk % 4));
  }
#endif

  while (ftell(in) < end_of_chunk)
  {
    read_bytes(in, chunk_id, 4);
    chunk_size = read_int32(in);
    end_of_subchunk = ftell(in) + chunk_size;

#if 0
    if ((end_of_subchunk % 4) != 0)
    {
      end_of_subchunk = end_of_subchunk + (4 - (end_of_subchunk % 4));
    }
#endif

    if (memcmp("JUNK", chunk_id, 4) == 0 || memcmp("PAD ", chunk_id, 4) == 0)
    {
      chunk_type[0] = 0;
    }
      else
    {
      read_bytes(in, chunk_type, 4);
    }

#ifdef DEBUG
    printf("New Chunk\n");
    printf("-------------------------------\n");
    printf("           chunk_id: %.4s\n", chunk_id);
    printf("         chunk_size: %d\n", chunk_size);
    printf("         chunk_type: %.4s\n", chunk_type);
    printf("\n");
    fflush(stdout);
#endif

    if (memcmp("JUNK", chunk_id, 4) == 0 || memcmp("PAD ", chunk_id, 4) == 0)
    {
      if ((chunk_size % 4) != 0)
      {
        chunk_size = chunk_size + (4 - (chunk_size % 4));
      }

      fseek(in, ftell(in) + chunk_size, SEEK_SET);
    }
      else
    if (strncasecmp("hdrl", chunk_type, 4) == 0)
    {
      parse_hdrl(in, &avi_header, &stream_header, &stream_format);
    }
      else
    if (strncasecmp("movi", chunk_type, 4) == 0)
    {
      movi_ptr=ftell(in) - 4;
    }
      else
    if (strncasecmp("idx1", chunk_id, 4) == 0)
    {
      fseek(in, ftell(in) - 4, SEEK_SET);
      parse_idx1(in, chunk_size, video, movi_ptr);
    }
      else
    {
#ifdef DEBUG
      printf("Unknown chunk at %d (%4s)\n", (int)ftell(in) - 8, chunk_type);
#endif
      if (chunk_size == 0) { break; }
    }

    fseek(in, end_of_subchunk, SEEK_SET);
  }

  float_fps = (float)1000000 / (float)avi_header.time_delay;
  video->fps = (int)float_fps;

  if (float_fps - (float)(int)float_fps >= 0.5) { video->fps++; }

  if (stream_format.palette != 0)
  {
    free(stream_format.palette);
  }

  return 0;
}

