/*

 rl2dbms -- DBMS related functions

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
#include <float.h>

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#include <spatialite/gaiaaux.h>

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

static void
prime_void_tile_int8 (void *pixels, unsigned short width, unsigned short height)
{
/* priming a void tile buffer - INT8 */
    int row;
    int col;
    char *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = 0;
      }
}

static void
prime_void_tile_uint8 (void *pixels, unsigned short width,
		       unsigned short height, unsigned char num_bands)
{
/* priming a void tile buffer - UINT8 */
    int row;
    int col;
    int band;
    unsigned char *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		    *p++ = 0;
	    }
      }
}

static void
prime_void_tile_int16 (void *pixels, unsigned short width,
		       unsigned short height)
{
/* priming a void tile buffer - INT16 */
    int row;
    int col;
    short *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = 0;
      }
}

static void
prime_void_tile_uint16 (void *pixels, unsigned short width,
			unsigned short height, unsigned char num_bands)
{
/* priming a void tile buffer - UINT16 */
    int row;
    int col;
    int band;
    unsigned short *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		    *p++ = 0;
	    }
      }
}

static void
prime_void_tile_int32 (void *pixels, unsigned short width,
		       unsigned short height)
{
/* priming a void tile buffer - INT32 */
    int row;
    int col;
    int *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = 0;
      }
}

static void
prime_void_tile_uint32 (void *pixels, unsigned short width,
			unsigned short height)
{
/* priming a void tile buffer - UINT32 */
    int row;
    int col;
    unsigned int *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = 0;
      }
}

static void
prime_void_tile_float (void *pixels, unsigned short width,
		       unsigned short height)
{
/* priming a void tile buffer - Float */
    int row;
    int col;
    float *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = 0.0;
      }
}

static void
prime_void_tile_double (void *pixels, unsigned short width,
			unsigned short height)
{
/* priming a void tile buffer - Double */
    int row;
    int col;
    double *p = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = 0.0;
      }
}

RL2_DECLARE void
rl2_prime_void_tile (void *pixels, unsigned short width, unsigned short height,
		     unsigned char sample_type, unsigned char num_bands)
{
/* priming a void tile buffer */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  prime_void_tile_int8 (pixels, width, height);
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  prime_void_tile_uint8 (pixels, width, height, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  prime_void_tile_int16 (pixels, width, height);
	  break;
      case RL2_SAMPLE_UINT16:
	  prime_void_tile_uint16 (pixels, width, height, num_bands);
	  break;
      case RL2_SAMPLE_INT32:
	  prime_void_tile_int32 (pixels, width, height);
	  break;
      case RL2_SAMPLE_UINT32:
	  prime_void_tile_uint32 (pixels, width, height);
	  break;
      case RL2_SAMPLE_FLOAT:
	  prime_void_tile_float (pixels, width, height);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  prime_void_tile_double (pixels, width, height);
	  break;
      };
}

static rl2PixelPtr
rescale_int8 (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
	      int size)
{
/* rescaling a square block of INT8 pixels */
    int x;
    int y;
    char *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (y = 0; y < size; y++)
      {
	  if (rst->maskBuffer != NULL)
	      pmsk = rst->maskBuffer + ((row + y) * rst->width) + col;
	  pxl = (char *) (rst->rasterBuffer + ((row + y) * rst->width) + col);
	  for (x = 0; x < size; x++)
	    {
		if (rst->maskBuffer != NULL)
		  {
		      if (*pmsk++ == 0)
			  goto transparent;
		  }
		sum += *pxl++;
	    }
      }
    rl2_set_pixel_sample_int8 (pixel, (char) rint (sum / (size * size)));
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

static rl2PixelPtr
rescale_uint8 (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
	       int size, int num_bands)
{
/* rescaling a square block of UINT8 pixels */
    int x;
    int y;
    int band;
    unsigned char *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (band = 0; band < num_bands; band++)
      {
	  for (y = 0; y < size; y++)
	    {
		if (rst->maskBuffer != NULL)
		    pmsk = rst->maskBuffer + ((row + y) * rst->width) + col;
		pxl =
		    (unsigned char *) (rst->rasterBuffer +
				       ((row + y) * rst->width * num_bands) +
				       (col * num_bands) + band);
		for (x = 0; x < size; x++)
		  {
		      if (rst->maskBuffer != NULL)
			{
			    if (*pmsk++ == 0)
				goto transparent;
			}
		      sum += *pxl;
		      pxl += num_bands;
		  }
	    }
	  rl2_set_pixel_sample_uint8 (pixel, band,
				      (unsigned char) rint (sum /
							    (size * size)));
      }
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

static rl2PixelPtr
rescale_int16 (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
	       int size)
{
/* rescaling a square block of INT16 pixels */
    int x;
    int y;
    short *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (y = 0; y < size; y++)
      {
	  if (rst->maskBuffer != NULL)
	      pmsk = rst->maskBuffer + ((row + y) * rst->width) + col;
	  pxl = (short *) (rst->rasterBuffer + ((row + y) * rst->width) + col);
	  for (x = 0; x < size; x++)
	    {
		if (rst->maskBuffer != NULL)
		  {
		      if (*pmsk++ == 0)
			  goto transparent;
		  }
		sum += *pxl++;
	    }
      }
    rl2_set_pixel_sample_int16 (pixel, (short) rint (sum / (size * size)));
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

static rl2PixelPtr
rescale_uint16 (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
		int size, int num_bands)
{
/* rescaling a square block of UINT16 pixels */
    int x;
    int y;
    int band;
    unsigned short *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (band = 0; band < num_bands; band++)
      {
	  for (y = 0; y < size; y++)
	    {
		if (rst->maskBuffer != NULL)
		    pmsk =
			rst->maskBuffer + ((row + y) * rst->width * num_bands) +
			col;
		pxl =
		    (unsigned short *) (rst->rasterBuffer +
					((row + y) * rst->width * num_bands) +
					(col * num_bands) + band);
		for (x = 0; x < size; x++)
		  {
		      if (rst->maskBuffer != NULL)
			{
			    if (*pmsk++ == 0)
				goto transparent;
			}
		      sum += *pxl;
		      pxl += num_bands;
		  }
	    }
	  rl2_set_pixel_sample_uint16 (pixel, band,
				       (unsigned short) rint (sum /
							      (size * size)));
      }
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

static rl2PixelPtr
rescale_int32 (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
	       int size)
{
/* rescaling a square block of INT32 pixels */
    int x;
    int y;
    int *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (y = 0; y < size; y++)
      {
	  if (rst->maskBuffer != NULL)
	      pmsk = rst->maskBuffer + ((row + y) * rst->width) + col;
	  pxl = (int *) (rst->rasterBuffer + ((row + y) * rst->width) + col);
	  for (x = 0; x < size; x++)
	    {
		if (rst->maskBuffer != NULL)
		  {
		      if (*pmsk++ == 0)
			  goto transparent;
		  }
		sum += *pxl++;
	    }
      }
    rl2_set_pixel_sample_int32 (pixel, (int) rint (sum / (size * size)));
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

static rl2PixelPtr
rescale_uint32 (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
		int size)
{
/* rescaling a square block of UINT32 pixels */
    int x;
    int y;
    unsigned int *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (y = 0; y < size; y++)
      {
	  if (rst->maskBuffer != NULL)
	      pmsk = rst->maskBuffer + ((row + y) * rst->width) + col;
	  pxl =
	      (unsigned int *) (rst->rasterBuffer + ((row + y) * rst->width) +
				col);
	  for (x = 0; x < size; x++)
	    {
		if (rst->maskBuffer != NULL)
		  {
		      if (*pmsk++ == 0)
			  goto transparent;
		  }
		sum += *pxl++;
	    }
      }
    rl2_set_pixel_sample_uint32 (pixel,
				 (unsigned int) rint (sum / (size * size)));
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

static rl2PixelPtr
rescale_float (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
	       int size)
{
/* rescaling a square block of FLOAT pixels */
    int x;
    int y;
    float *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (y = 0; y < size; y++)
      {
	  if (rst->maskBuffer != NULL)
	      pmsk = rst->maskBuffer + ((row + y) * rst->width) + col;
	  pxl = (float *) (rst->rasterBuffer + ((row + y) * rst->width) + col);
	  for (x = 0; x < size; x++)
	    {
		if (rst->maskBuffer != NULL)
		  {
		      if (*pmsk++ == 0)
			  goto transparent;
		  }
		sum += *pxl++;
	    }
      }
    rl2_set_pixel_sample_float (pixel, (float) (sum / (size * size)));
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

static rl2PixelPtr
rescale_double (rl2PixelPtr pixel, rl2PrivRasterPtr rst, int row, int col,
		int size)
{
/* rescaling a square block of DOUBLE pixels */
    int x;
    int y;
    double *pxl;
    unsigned char *pmsk;
    double sum = 0.0;

    for (y = 0; y < size; y++)
      {
	  if (rst->maskBuffer != NULL)
	      pmsk = rst->maskBuffer + ((row + y) * rst->width) + col;
	  pxl = (double *) (rst->rasterBuffer + ((row + y) * rst->width) + col);
	  for (x = 0; x < size; x++)
	    {
		if (rst->maskBuffer != NULL)
		  {
		      if (*pmsk++ == 0)
			  goto transparent;
		  }
		sum += *pxl++;
	    }
      }
    rl2_set_pixel_sample_double (pixel, sum / (size * size));
    return pixel;

  transparent:
    rl2_destroy_pixel (pixel);
    return NULL;
}

RL2_DECLARE rl2PixelPtr
rl2_rescale_block (rl2RasterPtr rst, int row, int col, int size)
{
/* rescaling a square block of pixels */
    rl2PrivRasterPtr raster = (rl2PrivRasterPtr) rst;
    rl2PixelPtr pixel = rl2_create_pixel (raster->sampleType, raster->pixelType,
					  raster->nBands);

    switch (raster->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  return rescale_int8 (pixel, raster, row, col, size);
      case RL2_SAMPLE_UINT8:
	  return rescale_uint8 (pixel, raster, row, col, size, raster->nBands);
      case RL2_SAMPLE_INT16:
	  return rescale_int16 (pixel, raster, row, col, size);
      case RL2_SAMPLE_UINT16:
	  return rescale_uint16 (pixel, raster, row, col, size, raster->nBands);
      case RL2_SAMPLE_INT32:
	  return rescale_int32 (pixel, raster, row, col, size);
      case RL2_SAMPLE_UINT32:
	  return rescale_uint32 (pixel, raster, row, col, size);
      case RL2_SAMPLE_FLOAT:
	  return rescale_float (pixel, raster, row, col, size);
      case RL2_SAMPLE_DOUBLE:
	  return rescale_double (pixel, raster, row, col, size);
      };
    rl2_destroy_pixel (pixel);
    return NULL;
}

RL2_DECLARE rl2CoveragePtr
rl2_create_coverage_from_dbms (sqlite3 * handle, const char *coverage)
{
/* attempting to create a Coverage Object from the DBMS definition */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    int sample;
    int pixel;
    int num_bands;
    int compression;
    int quality;
    int tile_width;
    int tile_height;
    double x_res;
    double y_res;
    int srid;
    int ok = 0;
    const char *value;
    rl2CoveragePtr cvg;

/* querying the Coverage metadata defs */
    sql =
	"SELECT sample_type, pixel_type, num_bands, compression, quality, tile_width, "
	"tile_height, horz_resolution, vert_resolution, srid, nodata_pixel "
	"FROM raster_coverages WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  return NULL;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_sample = 0;
		int ok_pixel = 0;
		int ok_num_bands = 0;
		int ok_compression = 0;
		int ok_quality = 0;
		int ok_tile_width = 0;
		int ok_tile_height = 0;
		int ok_x_res = 0;
		int ok_y_res = 0;
		int ok_srid = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 0);
		      if (strcasecmp (value, "1-BIT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_1_BIT;
			}
		      if (strcasecmp (value, "2-BIT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_2_BIT;
			}
		      if (strcasecmp (value, "4-BIT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_4_BIT;
			}
		      if (strcasecmp (value, "INT8") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_INT8;
			}
		      if (strcasecmp (value, "UINT8") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_UINT8;
			}
		      if (strcasecmp (value, "INT16") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_INT16;
			}
		      if (strcasecmp (value, "UINT16") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_UINT16;
			}
		      if (strcasecmp (value, "INT32") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_INT32;
			}
		      if (strcasecmp (value, "UINT32") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_UINT32;
			}
		      if (strcasecmp (value, "FLOAT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_FLOAT;
			}
		      if (strcasecmp (value, "DOUBLE") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_DOUBLE;
			}
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 1);
		      if (strcasecmp (value, "MONOCHROME") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_MONOCHROME;
			}
		      if (strcasecmp (value, "PALETTE") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_PALETTE;
			}
		      if (strcasecmp (value, "GRAYSCALE") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_GRAYSCALE;
			}
		      if (strcasecmp (value, "RGB") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_RGB;
			}
		      if (strcasecmp (value, "MULTIBAND") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_MULTIBAND;
			}
		      if (strcasecmp (value, "DATAGRID") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_DATAGRID;
			}
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		  {
		      num_bands = sqlite3_column_int (stmt, 2);
		      ok_num_bands = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 3);
		      if (strcasecmp (value, "NONE") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_NONE;
			}
		      if (strcasecmp (value, "DEFLATE") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_DEFLATE;
			}
		      if (strcasecmp (value, "LZMA") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LZMA;
			}
		      if (strcasecmp (value, "GIF") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_GIF;
			}
		      if (strcasecmp (value, "PNG") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_PNG;
			}
		      if (strcasecmp (value, "JPEG") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_JPEG;
			}
		      if (strcasecmp (value, "LOSSY_WEBP") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LOSSY_WEBP;
			}
		      if (strcasecmp (value, "LOSSLESS_WEBP") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LOSSLESS_WEBP;
			}
		  }
		if (sqlite3_column_type (stmt, 4) == SQLITE_INTEGER)
		  {
		      quality = sqlite3_column_int (stmt, 4);
		      ok_quality = 1;
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_INTEGER)
		  {
		      tile_width = sqlite3_column_int (stmt, 5);
		      ok_tile_width = 1;
		  }
		if (sqlite3_column_type (stmt, 6) == SQLITE_INTEGER)
		  {
		      tile_height = sqlite3_column_int (stmt, 6);
		      ok_tile_height = 1;
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT)
		  {
		      x_res = sqlite3_column_int (stmt, 7);
		      ok_x_res = 1;
		  }
		if (sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      y_res = sqlite3_column_int (stmt, 8);
		      ok_y_res = 1;
		  }
		if (sqlite3_column_type (stmt, 9) == SQLITE_INTEGER)
		  {
		      srid = sqlite3_column_int (stmt, 9);
		      ok_srid = 1;
		  }
		if (ok_sample && ok_pixel && ok_num_bands && ok_compression
		    && ok_quality && ok_tile_width && ok_tile_height && ok_x_res
		    && ok_y_res && ok_srid)
		    ok = 1;
	    }
      }
    sqlite3_finalize (stmt);

    if (!ok)
      {
	  fprintf (stderr, "ERROR: unable to find a Coverage named \"%s\"\n",
		   coverage);
	  return NULL;
      }

    cvg =
	rl2_create_coverage (coverage, sample, pixel, num_bands, compression,
			     quality, tile_width, tile_height, NULL);
    if (coverage == NULL)
      {
	  fprintf (stderr,
		   "ERROR: unable to create a Coverage Object supporting \"%s\"\n",
		   coverage);
	  return NULL;
      }
    if (rl2_coverage_georeference (cvg, srid, x_res, y_res) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to Georeference a Coverage Object supporting \"%s\"\n",
		   coverage);
	  rl2_destroy_coverage (cvg);
	  return NULL;
      }
    return cvg;
}

static void
void_int8_raw_buffer (char *buffer, unsigned short width, unsigned short height,
		      unsigned char num_bands)
{
/* preparing an empty/void INT8 raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    char *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0;
	    }
      }
}

static void
void_uint8_raw_buffer (unsigned char *buffer, unsigned short width,
		       unsigned short height, unsigned char num_bands)
{
/* preparing an empty/void UINT8 raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    unsigned char *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0;
	    }
      }
}

static void
void_int16_raw_buffer (short *buffer, unsigned short width,
		       unsigned short height, unsigned char num_bands)
{
/* preparing an empty/void INT16 raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    short *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0;
	    }
      }
}

static void
void_uint16_raw_buffer (unsigned short *buffer, unsigned short width,
			unsigned short height, unsigned char num_bands)
{
/* preparing an empty/void UINT16 raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    unsigned short *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0;
	    }
      }
}

static void
void_int32_raw_buffer (int *buffer, unsigned short width, unsigned short height,
		       unsigned char num_bands)
{
/* preparing an empty/void INT32 raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    int *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0;
	    }
      }
}

static void
void_uint32_raw_buffer (unsigned int *buffer, unsigned short width,
			unsigned short height, unsigned char num_bands)
{
/* preparing an empty/void UINT32 raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    unsigned int *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0;
	    }
      }
}

static void
void_float_raw_buffer (float *buffer, unsigned short width,
		       unsigned short height, unsigned char num_bands)
{
/* preparing an empty/void FLOAT raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    float *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0.0;
	    }
      }
}

static void
void_double_raw_buffer (double *buffer, unsigned short width,
			unsigned short height, unsigned char num_bands)
{
/* preparing an empty/void DOUBLE raw buffer */
    unsigned short x;
    unsigned short y;
    unsigned char b;
    double *p = buffer;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p++ = 0.0;
	    }
      }
}

static void
void_raw_buffer (unsigned char *buffer, unsigned short width,
		 unsigned short height, unsigned char sample_type,
		 unsigned char num_bands)
{
/* preparing an empty/void buffer initialized to all ZEROes */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  void_int8_raw_buffer ((char *) buffer, width, height, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  void_int16_raw_buffer ((short *) buffer, width, height, num_bands);
	  break;
      case RL2_SAMPLE_UINT16:
	  void_uint16_raw_buffer ((unsigned short *) buffer, width, height,
				  num_bands);
	  break;
      case RL2_SAMPLE_INT32:
	  void_int32_raw_buffer ((int *) buffer, width, height, num_bands);
	  break;
      case RL2_SAMPLE_UINT32:
	  void_uint32_raw_buffer ((unsigned int *) buffer, width, height,
				  num_bands);
	  break;
      case RL2_SAMPLE_FLOAT:
	  void_float_raw_buffer ((float *) buffer, width, height, num_bands);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  void_double_raw_buffer ((double *) buffer, width, height, num_bands);
	  break;
      default:
	  void_uint8_raw_buffer ((unsigned char *) buffer, width, height,
				 num_bands);
	  break;
      };
}

static void
copy_uint8_raw_pixels (const unsigned char *buffer, unsigned char *strip,
		       unsigned short width, unsigned short height,
		       unsigned char num_bands, double x_res, double y_res,
		       double minx, double maxy, double tile_minx,
		       double tile_maxy, unsigned short tile_width,
		       unsigned short tile_height)
{
/* copying UINT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    unsigned char *p_out;

    for (y = 0; y < tile_height; y++)
      {
	  geo_y = tile_maxy - ((double) y * y_res);
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		continue;
	    }
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x = tile_minx + ((double) x * x_res);
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      continue;
		  }
		p_out =
		    strip + (out_y * width * num_bands) + (out_x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static int
copy_raw_pixels (rl2RasterPtr raster, unsigned char *strip,
		 unsigned short width, unsigned short height,
		 unsigned char sample_type, unsigned char num_bands,
		 double x_res, double y_res, double minx, double maxy,
		 double tile_minx, double tile_maxy)
{
/* copying raw pixels into the output buffer */
    unsigned short tile_width;
    unsigned short tile_height;
    unsigned char *buffer = NULL;
    int buf_size;

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;

    switch (sample_type)
      {
	  /*
	     case RL2_SAMPLE_INT8:
	     return copy_int8_strip_pixels(raster, (char *)strip, width, height, num_bands, x_res, y_res, 
	     minx, miny, maxx, maxy, tile_minx, tile_miny, tile_maxx, tile_maxy); break;
	     case RL2_SAMPLE_INT16:
	     return copy_int16_strip_pixels(raster, (short *)strip, width, height, num_bands, x_res, y_res, 
	     minx, miny, maxx, maxy, tile_minx, tile_miny, tile_maxx, tile_maxy); break;
	     case RL2_SAMPLE_UINT16:
	     return copy_uint16_strip_pixels(raster, (unsigned short *)strip, width, height, num_bands, x_res, y_res, 
	     minx, miny, maxx, maxy, tile_minx, tile_miny, tile_maxx, tile_maxy); break;
	     case RL2_SAMPLE_INT32:
	     return copy_int32_strip_pixels(raster, (int *)strip, width, height, num_bands, x_res, y_res, 
	     minx, miny, maxx, maxy, tile_minx, tile_miny, tile_maxx, tile_maxy); break;
	     case RL2_SAMPLE_UINT32:
	     return copy_uint32_strip_pixels(raster, (unsigned int *)strip, width, height, num_bands, x_res, y_res, 
	     minx, miny, maxx, maxy, tile_minx, tile_miny, tile_maxx, tile_maxy); break;
	     case RL2_SAMPLE_FLOAT:
	     return copy_float_strip_pixels(raster, (float *)strip, width, height, num_bands, x_res, y_res, 
	     minx, miny, maxx, maxy, tile_minx, tile_miny, tile_maxx, tile_maxy); break;
	     case RL2_SAMPLE_DOUBLE:
	     return copy_double_strip_pixels(raster, (double *)strip, width, height, num_bands, x_res, y_res, 
	     minx, miny, maxx, maxy, tile_minx, tile_miny, tile_maxx, tile_maxy); break;
	   */
      default:
	  if (rl2_raster_data_to_RGB (raster, &buffer, &buf_size) != RL2_OK)
	      goto stop;
	  copy_uint8_raw_pixels (buffer, (unsigned char *) strip, width, height,
				 num_bands, x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height);
	  free (buffer);
	  return 1;
      };

  stop:
    if (buffer != NULL)
	free (buffer);
    return 0;
}

static int
load_dbms_tiles (sqlite3 * handle, sqlite3_stmt * stmt, unsigned char *strip,
		 unsigned short width, unsigned short height,
		 unsigned char sample_type, unsigned char num_bands,
		 double x_res, double y_res, double minx, double maxx,
		 double miny, double maxy, int level, int scale)
{
/* initializing a full strip */
    rl2RasterPtr raster = NULL;
    int ret;
    int scale_type = RL2_SCALE_1;
    switch (scale)
      {
      case 2:
	  scale_type = RL2_SCALE_2;
	  break;
      case 4:
	  scale_type = RL2_SCALE_4;
	  break;
      case 8:
	  scale_type = RL2_SCALE_8;
	  break;
      };

/* querying the section */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, level);
    sqlite3_bind_double (stmt, 2, minx);
    sqlite3_bind_double (stmt, 3, miny);
    sqlite3_bind_double (stmt, 4, maxx);
    sqlite3_bind_double (stmt, 5, maxy);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob_odd = NULL;
		int blob_odd_sz = 0;
		const unsigned char *blob_even = NULL;
		int blob_even_sz = 0;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt, 0);
		double tile_minx = sqlite3_column_double (stmt, 1);
		double tile_maxy = sqlite3_column_double (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_BLOB)
		  {
		      blob_odd = sqlite3_column_blob (stmt, 3);
		      blob_odd_sz = sqlite3_column_bytes (stmt, 3);
		  }
		if (sqlite3_column_type (stmt, 4) == SQLITE_BLOB)
		  {
		      blob_even = sqlite3_column_blob (stmt, 4);
		      blob_even_sz = sqlite3_column_bytes (stmt, 4);
		  }
		raster =
		    rl2_raster_decode (scale_type, blob_odd,
				       blob_odd_sz, blob_even, blob_even_sz);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      goto error;
		  }
		if (!copy_raw_pixels
		    (raster, strip, width, height, sample_type, num_bands,
		     x_res, y_res, minx, maxy, tile_minx, tile_maxy))
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT tiles; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }

    return 1;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return 0;
}

RL2_DECLARE int
rl2_find_matching_resolution (sqlite3 * handle, const char *coverage,
			      double *x_res, double *y_res, int *level,
			      int *scale)
{
/* attempting to identify the corresponding resolution level */
    int ret;
    int found = 0;
    int x_level;
    int x_scale;
    double z_x_res;
    double z_y_res;
    char *xcoverage;
    char *xxcoverage;
    char *sql;
    sqlite3_stmt *stmt = NULL;

    xcoverage = sqlite3_mprintf ("%s_levels", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    sql =
	sqlite3_mprintf
	("SELECT pyramid_level, x_resolution_1_1, y_resolution_1_1, "
	 "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, y_resolution_1_4, "
	 "x_resolution_1_8, y_resolution_1_8 FROM \"%s\"", xxcoverage);
    free (xxcoverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_free (sql);


    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		double xx_res;
		double yy_res;
		double confidence;
		int ok;
		int lvl = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 1);
		      yy_res = sqlite3_column_double (stmt, 2);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = 1;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 4) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 3);
		      yy_res = sqlite3_column_double (stmt, 4);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = 2;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 5);
		      yy_res = sqlite3_column_double (stmt, 6);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = 4;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 7);
		      yy_res = sqlite3_column_double (stmt, 8);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = 8;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (found)
      {
	  *level = x_level;
	  *scale = x_scale;
	  *x_res = z_x_res;
	  *y_res = z_y_res;
	  return RL2_OK;
      }
    return RL2_ERROR;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raw_raster_data (sqlite3 * handle, rl2CoveragePtr cvg,
			 unsigned short width, unsigned short height,
			 double minx, double miny, double maxx, double maxy,
			 double x_res, double y_res, unsigned char **buffer,
			 int *buf_size)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    const char *coverage;
    int level;
    int scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    char *sql;
    sqlite3_stmt *stmt;
    int ret;

    if (cvg == NULL || handle == NULL)
	goto error;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	goto error;
    if (rl2_find_matching_resolution
	(handle, coverage, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	goto error;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pix_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_sz = 8;
	  break;
      };
    bufpix_size = pix_sz * num_bands * width * height;
    bufpix = malloc (bufpix_size);
    if (bufpix == NULL)
      {
	  fprintf (stderr,
		   "rl2_get_raw_raster_data: Insufficient Memory !!!\n");
	  goto error;
      }

    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = gaiaDoubleQuotedSql (xtiles);
    xdata = sqlite3_mprintf ("%s_tile_data", coverage);
    xxdata = gaiaDoubleQuotedSql (xdata);
    sqlite3_free (xdata);
    sql = sqlite3_mprintf ("SELECT t.tile_id, MbrMinX(t.geometry), "
			   "MbrMaxY(t.geometry), d.tile_data_odd, d.tile_data_even "
			   "FROM \"%s\" AS t "
			   "JOIN \"%s\" AS d ON (d.tile_id = t.tile_id) "
			   "WHERE t.pyramid_level = ? AND t.ROWID IN ( "
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
			   "AND search_frame = BuildMBR(?, ?, ?, ?))", xxtiles,
			   xxdata, xtiles);
    sqlite3_free (xtiles);
    free (xxtiles);
    free (xxdata);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT raw tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

/* preparing a raw pixels buffer */
    void_raw_buffer (bufpix, width, height, sample_type, num_bands);
    if (!load_dbms_tiles
	(handle, stmt, bufpix, width, height, sample_type, num_bands, xx_res,
	 yy_res, minx, miny, maxx, maxy, level, scale))
	goto error;
    sqlite3_finalize (stmt);
    *buffer = bufpix;
    *buf_size = bufpix_size;
    return RL2_OK;

  error:
    if (bufpix != NULL)
	free (bufpix);
    return RL2_ERROR;
}
