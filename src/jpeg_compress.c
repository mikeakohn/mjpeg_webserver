/*

  mjpeg_webserver - Web server optimized for JPEGs from a webcam or AVI file.

  Copyright 2004-2024 - Michael Kohn (mike@mikekohn.net)
  https://www.mikekohn.net/

  This program falls under the GPLv3 license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>
#include <jpegint.h>
#include <jerror.h>

#include "jpeg_compress.h"

METHODDEF(void) init_destination (j_compress_ptr cinfo) { }
METHODDEF(boolean) empty_output_buffer (j_compress_ptr cinfo) { return TRUE; }
METHODDEF(void) term_destination (j_compress_ptr cinfo) { }

#ifdef NEED_DECOMPRESS
static void init_source(j_decompress_ptr cinfo) { }
static boolean fill_input_buffer(j_decompress_ptr cinfo) { return FALSE; }
static void term_source(j_decompress_ptr cinfo) { }

static void
skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  if ((size_t)num_bytes>cinfo->src->bytes_in_buffer)
  {
    cinfo->src->next_input_byte = NULL;
    cinfo->src->bytes_in_buffer = 0;
  }
    else
  {
    cinfo->src->next_input_byte += (size_t)num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t)num_bytes;
  }
}

int jpeg_decompress(
  uint8_t *jpeg_buffer,
  int jpeg_buffer_len,
  uint8_t *uncomp_buffer,
  int uncomp_buffer_len,
  int *width,
  int *height,
  int bytes_per_pixel)
{
  struct jpeg_decompress_struct decomp_cinfo;
  struct jpeg_error_mgr djerr;
  JSAMPARRAY samp_buffer;
  int bytes_per_row;

  decomp_cinfo.err = jpeg_std_error(&djerr);
  jpeg_create_decompress(&decomp_cinfo);

  decomp_cinfo.src = (struct jpeg_source_mgr *)
    (*decomp_cinfo.mem->alloc_small) ((j_common_ptr)&decomp_cinfo,
    JPOOL_PERMANENT,
    sizeof(struct jpeg_source_mgr));

  decomp_cinfo.src->init_source       = init_source;
  decomp_cinfo.src->fill_input_buffer = fill_input_buffer;
  decomp_cinfo.src->skip_input_data   = skip_input_data;
  decomp_cinfo.src->resync_to_restart = jpeg_resync_to_restart;
  decomp_cinfo.src->term_source       = term_source;
  decomp_cinfo.src->next_input_byte   = (JOCTET *)jpeg_buffer;
  decomp_cinfo.src->bytes_in_buffer   = jpeg_buffer_len;

  jpeg_read_header(&decomp_cinfo,TRUE);
  // decomp_cinfo.dct_method = JDCT_ISLOW;

  *width = decomp_cinfo.image_width;
  *height = decomp_cinfo.image_height;

  if (bytes_per_pixel == 1)
  {
    decomp_cinfo.out_color_space = JCS_GRAYSCALE;
    bytes_per_row = decomp_cinfo.image_width;
  }
    else
  { bytes_per_row = decomp_cinfo.image_width * 3; }

  jpeg_start_decompress(&decomp_cinfo);

  samp_buffer = (*decomp_cinfo.mem->alloc_sarray)
    ((j_common_ptr)&decomp_cinfo, JPOOL_IMAGE, bytes_per_row, 1);

  while (decomp_cinfo.output_scanline < decomp_cinfo.output_height)
  {
    jpeg_read_scanlines(&decomp_cinfo,samp_buffer,1);
    memcpy((JSAMPLE *)uncomp_buffer,samp_buffer[0],bytes_per_row);

    uncomp_buffer=uncomp_buffer+bytes_per_row;
  }

  jpeg_finish_decompress(&decomp_cinfo);
  jpeg_destroy_decompress(&decomp_cinfo);

  return 0;
}
#endif

int jpeg_compress(
  uint8_t *uncomp_buffer,
  int uncomp_buffer_len,
  uint8_t *jpeg_buffer,
  int jpeg_buffer_len,
  int width,
  int height,
  int bytes_per_pixel,
  int quality)
{
  struct jpeg_compress_struct comp_cinfo;
  struct jpeg_error_mgr cjerr;
  JSAMPROW row_pointer[1];
  int bytes_per_row;
  int jpeg_len;

  comp_cinfo.err = jpeg_std_error(&cjerr);
  jpeg_create_compress(&comp_cinfo);

  comp_cinfo.image_width=width;
  comp_cinfo.image_height=height;

  if (bytes_per_pixel==3)
  {
    comp_cinfo.input_components=3;
    comp_cinfo.in_color_space=JCS_RGB;
    bytes_per_row=width*3;
  }
    else
  {
    bytes_per_row=width;
    comp_cinfo.input_components=1;
    comp_cinfo.in_color_space=JCS_GRAYSCALE;
  }

  if (comp_cinfo.dest == NULL)
  {
    comp_cinfo.dest=(struct jpeg_destination_mgr *)(*comp_cinfo.mem->alloc_small) ((j_common_ptr) &comp_cinfo, JPOOL_PERMANENT,sizeof(struct jpeg_destination_mgr));
  }

  comp_cinfo.dest->next_output_byte = (JOCTET *)jpeg_buffer;
  comp_cinfo.dest->free_in_buffer = jpeg_buffer_len;
  comp_cinfo.dest->init_destination = init_destination;
  comp_cinfo.dest->empty_output_buffer = empty_output_buffer;
  comp_cinfo.dest->term_destination = term_destination;

  jpeg_set_defaults(&comp_cinfo);

  if (quality<1) quality=1;
    else
  if (quality>100) quality=100;

  jpeg_set_quality(&comp_cinfo,quality,TRUE);

  jpeg_start_compress(&comp_cinfo,TRUE);

  while(comp_cinfo.next_scanline<comp_cinfo.image_height)
  {
    row_pointer[0]=(JSAMPLE*)&uncomp_buffer[comp_cinfo.next_scanline*bytes_per_row];
    jpeg_write_scanlines(&comp_cinfo,row_pointer,1);
  }

  jpeg_finish_compress(&comp_cinfo);
  jpeg_len=jpeg_buffer_len-comp_cinfo.dest->free_in_buffer;

  jpeg_destroy_compress(&comp_cinfo);

  return jpeg_len;
}

