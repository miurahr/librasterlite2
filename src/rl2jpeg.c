/*

 rl2jpeg -- JPEG related functions

 version 0.1, 2013 March 29

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
the Initial Developer. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jpeglib.h>
#include <jerror.h>

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

typedef struct
{
    struct jpeg_destination_mgr pub;	/* public fields */

    unsigned char **outbuffer;	/* target buffer */
    unsigned long *outsize;
    unsigned char *newbuffer;	/* newly allocated buffer */
    JOCTET *buffer;		/* start of buffer */
    size_t bufsize;
    boolean alloc;
} jpeg_mem_destination_mgr;
typedef jpeg_mem_destination_mgr *jpeg_mem_dest_ptr;

static int
empty_mem_output_buffer (j_compress_ptr cinfo)
{
/*
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * Modified 2009-2010 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
*/
    size_t nextsize;
    JOCTET *nextbuffer;
    jpeg_mem_dest_ptr dest = (jpeg_mem_dest_ptr) cinfo->dest;

    if (!dest->alloc)
	ERREXIT (cinfo, JERR_BUFFER_SIZE);

    /* Try to allocate new buffer with double size */
    nextsize = dest->bufsize * 2;
    nextbuffer = malloc (nextsize);

    if (nextbuffer == NULL)
	ERREXIT1 (cinfo, JERR_OUT_OF_MEMORY, 10);

    memcpy (nextbuffer, dest->buffer, dest->bufsize);

    if (dest->newbuffer != NULL)
	free (dest->newbuffer);

    dest->newbuffer = nextbuffer;

    dest->pub.next_output_byte = nextbuffer + dest->bufsize;
    dest->pub.free_in_buffer = dest->bufsize;

    dest->buffer = nextbuffer;
    dest->bufsize = nextsize;

    return 1;
}

static void
init_mem_destination (j_compress_ptr cinfo)
{
/* just silencing stupid compiler warnings */
    if (cinfo == NULL)
	cinfo = NULL;
}

static void
term_mem_destination (j_compress_ptr cinfo)
{
/*
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * Modified 2009-2010 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
*/
    jpeg_mem_dest_ptr dest = (jpeg_mem_dest_ptr) cinfo->dest;

    if (dest->alloc)
	*dest->outbuffer = dest->buffer;
    *dest->outsize = (unsigned long) (dest->bufsize - dest->pub.free_in_buffer);
}

static int
fill_mem_input_buffer (j_decompress_ptr cinfo)
{
/*
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * Modified 2009-2010 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
*/
    static JOCTET mybuffer[4];

    /* The whole JPEG data is expected to reside in the supplied memory
     * buffer, so any request for more data beyond the given buffer size
     * is treated as an error.
     */
    WARNMS (cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    mybuffer[0] = (JOCTET) 0xFF;
    mybuffer[1] = (JOCTET) JPEG_EOI;

    cinfo->src->next_input_byte = mybuffer;
    cinfo->src->bytes_in_buffer = 2;

    return 1;
}

static void
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
/*
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * Modified 2009-2010 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
*/
    struct jpeg_source_mgr *src = cinfo->src;

    /* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
     * any trouble anyway --- large skips are infrequent.
     */
    if (num_bytes > 0)
      {
	  while (num_bytes > (long) src->bytes_in_buffer)
	    {
		num_bytes -= (long) src->bytes_in_buffer;
		(void) (*src->fill_input_buffer) (cinfo);
		/* note we assume that fill_input_buffer will never return FALSE,
		 * so suspension need not be handled.
		 */
	    }
	  src->next_input_byte += (size_t) num_bytes;
	  src->bytes_in_buffer -= (size_t) num_bytes;
      }
}

static void
term_source (j_decompress_ptr cinfo)
{
/* just silencing stupid compiler warnings */
    if (cinfo == NULL)
	cinfo = NULL;
}

static void
init_mem_source (j_decompress_ptr cinfo)
{
/* just silencing stupid compiler warnings */
    if (cinfo == NULL)
	cinfo = NULL;
}

static void
rl2_jpeg_src (j_decompress_ptr cinfo,
	      unsigned char *inbuffer, unsigned long insize)
{
/*
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * Modified 2009-2010 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
*/
    struct jpeg_source_mgr *src;

    if (inbuffer == NULL || insize == 0)	/* Treat empty input as fatal error */
	ERREXIT (cinfo, JERR_INPUT_EMPTY);

    /* The source object is made permanent so that a series of JPEG images
     * can be read from the same buffer by calling jpeg_mem_src only before
     * the first one.
     */
    if (cinfo->src == NULL)
      {				/* first time for this JPEG object? */
	  cinfo->src = (struct jpeg_source_mgr *)
	      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
					  sizeof (struct jpeg_source_mgr));
      }

    src = cinfo->src;
    src->init_source = init_mem_source;
    src->fill_input_buffer = fill_mem_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart;	/* use default method */
    src->term_source = term_source;
    src->bytes_in_buffer = (size_t) insize;
    src->next_input_byte = (JOCTET *) inbuffer;
}

static void
rl2_jpeg_dest (j_compress_ptr cinfo,
	       unsigned char **outbuffer, unsigned long *outsize)
{
/*
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * Modified 2009-2010 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
*/
    jpeg_mem_dest_ptr dest;

    if (outbuffer == NULL || outsize == NULL)	/* sanity check */
	ERREXIT (cinfo, JERR_BUFFER_SIZE);

    /* The destination object is made permanent so that multiple JPEG images
     * can be written to the same buffer without re-executing jpeg_mem_dest.
     */
    if (cinfo->dest == NULL)
      {				/* first time for this JPEG object? */
	  cinfo->dest = (struct jpeg_destination_mgr *)
	      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
					  sizeof (jpeg_mem_destination_mgr));
	  dest = (jpeg_mem_dest_ptr) cinfo->dest;
	  dest->newbuffer = NULL;
      }

    dest = (jpeg_mem_dest_ptr) cinfo->dest;
    dest->pub.init_destination = init_mem_destination;
    dest->pub.empty_output_buffer = empty_mem_output_buffer;
    dest->pub.term_destination = term_mem_destination;
    dest->outbuffer = outbuffer;
    dest->outsize = outsize;
    dest->alloc = 1;

    if (*outbuffer == NULL || *outsize == 0)
      {
	  /* Allocate initial buffer */
	  dest->newbuffer = *outbuffer = malloc (OUTPUT_BUF_SIZE);
	  if (dest->newbuffer == NULL)
	      ERREXIT1 (cinfo, JERR_OUT_OF_MEMORY, 10);
	  *outsize = OUTPUT_BUF_SIZE;
      }

    dest->pub.next_output_byte = dest->buffer = *outbuffer;
    dest->pub.free_in_buffer = dest->bufsize = *outsize;
}

static void
CMYK2RGB (int c, int m, int y, int k, int inverted, unsigned char *p)
{
/* converting from CMYK to RGB */
    if (inverted)
      {
	  c = 255 - c;
	  m = 255 - m;
	  y = 255 - y;
	  k = 255 - k;
      }
    *p++ = (255 - c) * (255 - k) / 255;
    *p++ = (255 - m) * (255 - k) / 255;
    *p++ = (255 - y) * (255 - k) / 255;
}

static int
check_jpeg_compatibility (unsigned char sample_type, unsigned char pixel_type,
			  unsigned char num_samples)
{
/* checks for JPEG compatibility */
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  break;
      default:
	  return RL2_ERROR;
      };
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
	  break;
      default:
	  return RL2_ERROR;
      };
    if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_RGB)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 3)
	      return RL2_ERROR;
      }
    return RL2_OK;
}

static int
compress_jpeg (rl2RasterPtr ptr, unsigned char **jpeg, int *jpeg_size,
	       int quality)
{
/* compressing a JPG image */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *outbuffer = NULL;
    unsigned long outsize = 0;
    volatile JSAMPROW scanline = NULL;
    JSAMPROW rowptr[1];
    JSAMPROW p_row;
    const char *comment;
    unsigned char *p_data;
    unsigned char *p_mask;
    int row;
    int col;
    unsigned short num_entries;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    unsigned char *alpha = NULL;

    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_compress (&cinfo);
    rl2_jpeg_dest (&cinfo, &outbuffer, &outsize);
    cinfo.image_width = rst->width;
    cinfo.image_height = rst->height;
    if (rst->pixelType == RL2_PIXEL_MONOCHROME
	|| rst->pixelType == RL2_PIXEL_GRAYSCALE)
      {
	  /* GRAYSCALE */
	  cinfo.input_components = 1;
	  cinfo.in_color_space = JCS_GRAYSCALE;
      }
    else
      {
	  /* RGB */
	  cinfo.input_components = 3;
	  cinfo.in_color_space = JCS_RGB;
      }
    jpeg_set_defaults (&cinfo);
    if (quality > 100)
	quality = 100;
    if (quality < 0)
	quality = 75;
    jpeg_set_quality (&cinfo, quality, 1);
    scanline =
	(JSAMPROW) calloc (1,
			   cinfo.image_width * cinfo.input_components *
			   sizeof (JSAMPLE));
    if (scanline == NULL)
	goto error;
    rowptr[0] = scanline;
    jpeg_start_compress (&cinfo, 1);
    comment = "CREATOR: RasterLite2\n";
    jpeg_write_marker (&cinfo, JPEG_COM, (unsigned char *) comment,
		       (unsigned int) strlen (comment));
    p_data = rst->rasterBuffer;
    p_mask = rst->maskBuffer;
    if (rst->pixelType == RL2_PIXEL_PALETTE)
      {
	  /* retrieving the palette */
	  if (rl2_get_palette_colors
	      ((rl2PalettePtr) (rst->Palette), &num_entries, &red, &green,
	       &blue, &alpha) != RL2_OK)
	      goto error;
      }
    for (row = 0; row < (int) (rst->height); row++)
      {
	  p_row = scanline;
	  for (col = 0; col < (int) (rst->width); col++)
	    {
		int transparent = 1;
		if (p_mask != NULL)
		    transparent = *p_mask++;
		transparent = !transparent;
		if (rst->pixelType == RL2_PIXEL_PALETTE)
		  {
		      unsigned char index = *p_data++;
		      if (transparent || *(alpha + index) == 0)
			{
			    /* transparent pixel - defaulting to WHITE */
			    *p_row++ = 255;
			    *p_row++ = 255;
			    *p_row++ = 255;
			}
		      else
			{
			    /* opaque pixel */
			    if (index >= num_entries)
			      {
				  /* color mismatch: default BLACK pixel */
				  *p_row++ = 0;
				  *p_row++ = 0;
				  *p_row++ = 0;
			      }
			    else
			      {
				  *p_row++ = *(red + index);
				  *p_row++ = *(green + index);
				  *p_row++ = *(blue + index);
			      }
			}
		  }
		else if (rst->pixelType == RL2_PIXEL_GRAYSCALE)
		  {
		      if (transparent)
			{
			    /* transparent pixel - defaulting to WHITE */
			    *p_row++ = 255;
			    p_data++;
			}
		      else
			{
			    /* opaque pixel */
			    switch (rst->sampleType)
			      {
			      case RL2_SAMPLE_2_BIT:
				  switch (*p_data++)
				    {
				    case 3:
					*p_row++ = 255;
					break;
				    case 2:
					*p_row++ = 170;
					break;
				    case 1:
					*p_row++ = 86;
					break;
				    case 0:
				    default:
					*p_row++ = 0;
					break;
				    };
				  break;
			      case RL2_SAMPLE_4_BIT:
				  switch (*p_data++)
				    {
				    case 15:
					*p_row++ = 255;
					break;
				    case 14:
					*p_row++ = 239;
					break;
				    case 13:
					*p_row++ = 222;
					break;
				    case 12:
					*p_row++ = 205;
					break;
				    case 11:
					*p_row++ = 188;
					break;
				    case 10:
					*p_row++ = 171;
					break;
				    case 9:
					*p_row++ = 154;
					break;
				    case 8:
					*p_row++ = 137;
					break;
				    case 7:
					*p_row++ = 119;
					break;
				    case 6:
					*p_row++ = 102;
					break;
				    case 5:
					*p_row++ = 85;
					break;
				    case 4:
					*p_row++ = 68;
					break;
				    case 3:
					*p_row++ = 51;
					break;
				    case 2:
					*p_row++ = 34;
					break;
				    case 1:
					*p_row++ = 17;
					break;
				    case 0:
				    default:
					*p_row++ = 0;
					break;
				    };
				  break;
			      case RL2_SAMPLE_UINT8:
			      default:
				  *p_row++ = *p_data++;
				  break;
			      };
			}
		  }
		else if (rst->pixelType == RL2_PIXEL_MONOCHROME)
		  {
		      if (transparent)
			{
			    /* transparent pixel - defaulting to WHITE */
			    *p_row++ = 255;
			    p_data++;
			}
		      else
			{
			    /* opaque pixel */
			    if (*p_data++ == 0)
				*p_row++ = 255;
			    else
				*p_row++ = 0;
			}
		  }
		else
		  {
		      /* RGB */
		      if (transparent)
			{
			    /* transparent pixel - defaulting to WHITE */
			    *p_row++ = 255;
			    *p_row++ = 255;
			    *p_row++ = 255;
			    p_data++;
			    p_data++;
			    p_data++;
			}
		      else
			{
			    /* opaque pixel */
			    *p_row++ = *p_data++;
			    *p_row++ = *p_data++;
			    *p_row++ = *p_data++;
			}
		  }
	    }
	  jpeg_write_scanlines (&cinfo, rowptr, 1);
      }
    jpeg_finish_compress (&cinfo);
    jpeg_destroy_compress (&cinfo);
    free (scanline);
    if (red != NULL)
	free (red);
    if (green != NULL)
	free (green);
    if (blue != NULL)
	free (blue);
    if (alpha != NULL)
	free (alpha);
    *jpeg = outbuffer;
    *jpeg_size = outsize;
    return RL2_OK;
  error:
    jpeg_destroy_compress (&cinfo);
    if (scanline != NULL)
	free (scanline);
    if (outbuffer != NULL)
	free (outbuffer);
    if (red != NULL)
	free (red);
    if (green != NULL)
	free (green);
    if (blue != NULL)
	free (blue);
    if (alpha != NULL)
	free (alpha);
    return RL2_ERROR;
}

RL2_PRIVATE int
rl2_blob_from_file (const char *path, unsigned char **p_blob, int *p_blob_size)
{
/* attempting to load a BLOB from a file */
    int rd;
    int blob_size;
    unsigned char *blob;
    FILE *in;

    *p_blob = NULL;
    *p_blob_size = 0;
/* attempting to open the file */
    in = fopen (path, "rb");
    if (in == NULL)
	return RL2_ERROR;

/* querying the file length */
    if (fseek (in, 0, SEEK_END) < 0)
	return RL2_ERROR;
    blob_size = ftell (in);
    rewind (in);
    blob = malloc (blob_size);
    if (blob == NULL)
      {
	  fclose (in);
	  return RL2_ERROR;
      }

/* attempting to load the BLOB from the file */
    rd = fread (blob, 1, blob_size, in);
    fclose (in);
    if (rd != blob_size)
      {
	  /* read error */
	  free (blob);
	  return RL2_ERROR;
      }
    *p_blob = blob;
    *p_blob_size = blob_size;
    return RL2_OK;
}

RL2_PRIVATE int
rl2_blob_to_file (const char *path, unsigned char *blob, int blob_size)
{
/* attempting to store a BLOB into a file */
    int wr;
    FILE *out;

    if (blob == NULL || blob_size < 1)
	return RL2_ERROR;
/* attempting to open the file */
    out = fopen (path, "wb");
    if (out == NULL)
	return RL2_ERROR;

/* attempting to load the BLOB from the file */
    wr = fwrite (blob, 1, blob_size, out);
    fclose (out);
    if (wr != blob_size)
      {
	  /* write error */
	  return RL2_ERROR;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_section_to_jpeg (rl2SectionPtr scn, const char *path, int quality)
{
/* attempting to save a raster section into a JPEG file */
    int blob_size;
    unsigned char *blob;
    rl2RasterPtr rst;
    int ret;

    if (scn == NULL)
	return RL2_ERROR;
    rst = rl2_get_section_raster (scn);
    if (rst == NULL)
	return RL2_ERROR;
/* attempting to export as a PNG image */
    if (rl2_raster_to_jpeg (rst, &blob, &blob_size, quality) != RL2_OK)
	return RL2_ERROR;
    ret = rl2_blob_to_file (path, blob, blob_size);
    free (blob);
    if (ret != RL2_OK)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_to_jpeg (rl2RasterPtr rst, unsigned char **jpeg, int *jpeg_size,
		    int quality)
{
/* creating a JPEG image from a raster */
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_samples;
    unsigned char *blob;
    int blob_size;

    if (rst == NULL)
	return RL2_ERROR;
    if (rl2_get_raster_type (rst, &sample_type, &pixel_type, &num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (check_jpeg_compatibility (sample_type, pixel_type, num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (compress_jpeg (rst, &blob, &blob_size, quality) != RL2_OK)
	return RL2_ERROR;
    *jpeg = blob;
    *jpeg_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE rl2SectionPtr
rl2_section_from_jpeg (const char *path)
{
/* attempting to create a raster section from a JPEG file */
    int blob_size;
    unsigned char *blob;
    rl2SectionPtr scn;
    rl2RasterPtr rst;

/* attempting to create a raster */
    if (rl2_blob_from_file (path, &blob, &blob_size) != RL2_OK)
	return NULL;
    rst = rl2_raster_from_jpeg (blob, blob_size);
    free (blob);
    if (rst == NULL)
	return NULL;

/* creating the raster section */
    scn =
	rl2_create_section (path, RL2_COMPRESSION_JPEG, RL2_TILESIZE_UNDEFINED,
			    RL2_TILESIZE_UNDEFINED, rst);
    return scn;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_from_jpeg (unsigned char *jpeg, int jpeg_size)
{
/* attempting to create a raster from a JPEG image */
    rl2RasterPtr rst = NULL;
    unsigned char *data = NULL;
    int data_size;
    unsigned short width;
    unsigned short height;
    unsigned char pixel_type;
    int nBands;

    if (rl2_decode_jpeg_scaled
	(1, jpeg, jpeg_size, &width, &height, &pixel_type, &data,
	 &data_size) != RL2_OK)
	goto error;
    nBands = 1;
    if (pixel_type == RL2_PIXEL_RGB)
	nBands = 3;

/* creating the raster */
    rst =
	rl2_create_raster (width, height, RL2_SAMPLE_UINT8, pixel_type, nBands,
			   data, data_size, NULL, NULL, 0, NULL);
    if (rst == NULL)
	goto error;
    return rst;

  error:
    if (rst != NULL)
	rl2_destroy_raster (rst);
    if (data != NULL)
	free (data);
    return NULL;
}


RL2_PRIVATE int
rl2_decode_jpeg_scaled (int scale, const unsigned char *jpeg, int jpeg_size,
			unsigned short *width, unsigned short *height,
			unsigned char *xpixel_type, unsigned char **pixels,
			int *pixels_size)
{
/* attempting to create a raster from a JPEG image - supporting rescaled size */
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    int nBands;
    int channels;
    int inverted = 0;
    unsigned char *data = NULL;
    unsigned char *p_data;
    int data_size;
    int i;
    int row_stride;
    JSAMPARRAY buffer;

    if (scale == 1 || scale == 2 || scale == 4 || scale == 8)
	;
    else
	goto error;

    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_decompress (&cinfo);
    rl2_jpeg_src (&cinfo, (unsigned char *) jpeg, jpeg_size);
    jpeg_read_header (&cinfo, TRUE);
    if (scale == 8)
      {
	  /* requesting 1:8 scaling */
	  cinfo.scale_num = 1;
      }
    else if (scale == 4)
      {
	  /* requesting 1:4 scaling */
	  cinfo.scale_num = 2;
      }
    else if (scale == 2)
      {
	  /* requesting 1:2 scaling */
	  cinfo.scale_num = 4;
      }
    else
      {
	  /* no scaling, full dimension */
	  cinfo.scale_num = 8;
      }
    cinfo.scale_denom = 8;
    if ((cinfo.jpeg_color_space == JCS_CMYK)
	|| (cinfo.jpeg_color_space == JCS_YCCK))
	cinfo.out_color_space = JCS_CMYK;
    if (!jpeg_start_decompress (&cinfo))
	goto error;
    channels = cinfo.output_components;
    if (cinfo.out_color_space == JCS_RGB && channels == 3)
      {
	  pixel_type = RL2_PIXEL_RGB;
	  nBands = 3;
      }
    else if (cinfo.out_color_space == JCS_GRAYSCALE && channels == 1)
      {
	  pixel_type = RL2_PIXEL_GRAYSCALE;
	  nBands = 1;
      }
    else if (cinfo.out_color_space == JCS_CMYK && channels == 4)
      {
	  jpeg_saved_marker_ptr marker;
	  pixel_type = RL2_PIXEL_RGB;
	  nBands = 3;
	  marker = cinfo.marker_list;
	  while (marker)
	    {
		if ((marker->marker == (JPEG_APP0 + 14))
		    && (marker->data_length >= 12)
		    && (!strncmp ((const char *) marker->data, "Adobe", 5)))
		  {
		      inverted = 1;
		      break;
		  }
		marker = marker->next;
	    }
      }
    else
	goto error;
/* creating the scanline buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    buffer =
	(*cinfo.mem->alloc_sarray) ((j_common_ptr) & cinfo, JPOOL_IMAGE,
				    row_stride, 1);
    if (buffer == NULL)
	goto error;
/* creating the raster data */
    data_size = cinfo.output_width * cinfo.output_height * nBands;
    data = malloc (data_size);
    if (data == NULL)
	goto error;
    p_data = data;
    while (cinfo.output_scanline < cinfo.output_height)
      {
	  /* reading all decompressed scanlines */
	  jpeg_read_scanlines (&cinfo, buffer, 1);
	  if (cinfo.out_color_space == JCS_CMYK)
	    {
		JSAMPROW row = buffer[0];
		for (i = 0; i < (int) (cinfo.output_width); i++)
		  {
		      CMYK2RGB (*(row + 0), *(row + 1), *(row + 2), *(row + 3),
				inverted, p_data);
		      row += 4;
		      p_data += 3;
		  }
	    }
	  else if (cinfo.out_color_space == JCS_GRAYSCALE)
	    {
		JSAMPROW row = buffer[0];
		for (i = 0; i < (int) (cinfo.output_width); i++)
		    *p_data++ = *row++;
	    }
	  else
	    {
		/* RGB */
		JSAMPROW row = buffer[0];
		for (i = 0; i < (int) (cinfo.output_width); i++)
		  {
		      *p_data++ = *row++;
		      *p_data++ = *row++;
		      *p_data++ = *row++;
		  }
	    }
      }
    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *xpixel_type = pixel_type;
    *pixels = data;
    *pixels_size = data_size;
/* memory cleanup */
    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);
    return RL2_OK;

  error:
    jpeg_destroy_decompress (&cinfo);
    if (data != NULL)
	free (data);
    return RL2_ERROR;
}
