/*

 rl2sql -- main SQLite extension methods

 version 0.1, 2013 December 22

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
#include <stdint.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#define RL2_UNUSED() if (argc || argv) argc = argc;

static void
fnct_rl2_version (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_version()
/
/ return a text string representing the current RasterLite-2 version
*/
    int len;
    const char *p_result = rl2_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_target_cpu (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_target_cpu()
/
/ return a text string representing the current RasterLite-2 Target CPU
*/
    int len;
    const char *p_result = rl2_target_cpu ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_IsValidPixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsValidPixel(BLOBencoded pixel, text sample_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *sample;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	sqlite3_result_int (context, -1);
    else
      {
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  bands = sqlite3_value_int (argv[2]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
	  ret = rl2_is_valid_dbms_pixel (blob, blob_sz, sample_type, num_bands);
	  if (ret == RL2_OK)
	      sqlite3_result_int (context, 1);
	  else
	      sqlite3_result_int (context, 0);
      }
}

static void
fnct_IsValidRasterPalette (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterPalette(BLOBencoded palette, text sample_type, text pixel_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *sample;
    const char *pixel;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	sqlite3_result_int (context, -1);
    else
      {
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  pixel = (const char *) sqlite3_value_text (argv[2]);
	  bands = sqlite3_value_int (argv[3]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (strcmp (pixel, "MONOCHROME") == 0)
	      pixel_type = RL2_PIXEL_MONOCHROME;
	  if (strcmp (pixel, "GRAYSCALE") == 0)
	      pixel_type = RL2_PIXEL_GRAYSCALE;
	  if (strcmp (pixel, "PALETTE") == 0)
	      pixel_type = RL2_PIXEL_PALETTE;
	  if (strcmp (pixel, "RGB") == 0)
	      pixel_type = RL2_PIXEL_RGB;
	  if (strcmp (pixel, "MULTIBAND") == 0)
	      pixel_type = RL2_PIXEL_MULTIBAND;
	  if (strcmp (pixel, "DATAGRID") == 0)
	      pixel_type = RL2_PIXEL_DATAGRID;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || pixel_type == RL2_PIXEL_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
	  ret =
	      rl2_is_valid_dbms_palette (blob, blob_sz, sample_type, pixel_type,
					 num_bands);
	  if (ret == RL2_OK)
	      sqlite3_result_int (context, 1);
	  else
	      sqlite3_result_int (context, 0);
      }
}

static int
get_coverage_sample_bands (sqlite3 * sqlite, const char *coverage,
			   unsigned char *sample_type, unsigned char *num_bands)
{
/* attempting to retrieve the SampleType and NumBands from a Coverage */
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    int ret;
    const char *sample;
    int bands;
    unsigned char xsample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char xnum_bands = RL2_BANDS_UNKNOWN;

    sql =
	sqlite3_mprintf ("SELECT sample_type, num_bands FROM raster_coverages "
			 "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		sample = results[(i * columns) + 0];
		if (strcmp (sample, "1-BIT") == 0)
		    xsample_type = RL2_SAMPLE_1_BIT;
		if (strcmp (sample, "2-BIT") == 0)
		    xsample_type = RL2_SAMPLE_2_BIT;
		if (strcmp (sample, "4-BIT") == 0)
		    xsample_type = RL2_SAMPLE_4_BIT;
		if (strcmp (sample, "INT8") == 0)
		    xsample_type = RL2_SAMPLE_INT8;
		if (strcmp (sample, "UINT8") == 0)
		    xsample_type = RL2_SAMPLE_UINT8;
		if (strcmp (sample, "INT16") == 0)
		    xsample_type = RL2_SAMPLE_INT16;
		if (strcmp (sample, "UINT16") == 0)
		    xsample_type = RL2_SAMPLE_UINT16;
		if (strcmp (sample, "INT32") == 0)
		    xsample_type = RL2_SAMPLE_INT32;
		if (strcmp (sample, "UINT32") == 0)
		    xsample_type = RL2_SAMPLE_UINT32;
		if (strcmp (sample, "FLOAT") == 0)
		    xsample_type = RL2_SAMPLE_FLOAT;
		if (strcmp (sample, "DOUBLE") == 0)
		    xsample_type = RL2_SAMPLE_DOUBLE;
		bands = atoi (results[(i * columns) + 1]);
		if (bands > 0 && bands < 256)
		    xnum_bands = bands;
	    }
      }
    sqlite3_free_table (results);
    if (xsample_type == RL2_SAMPLE_UNKNOWN || xnum_bands == RL2_BANDS_UNKNOWN)
	return 0;
    *sample_type = xsample_type;
    *num_bands = xnum_bands;
    return 1;
}

static void
fnct_IsValidRasterStatistics (sqlite3_context * context, int argc,
			      sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterStatistics(text covarage, BLOBencoded statistics)
/   or
/ IsValidRasterStatistics((BLOBencoded statistics, text sample_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *coverage;
    const char *sample;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    sqlite3 *sqlite;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (argc == 3)
      {
	  if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (err)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  bands = sqlite3_value_int (argv[2]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
      }
    else
      {
	  if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	      err = 1;
	  if (err)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
	  sqlite = sqlite3_context_db_handle (context);
	  coverage = (const char *) sqlite3_value_text (argv[0]);
	  blob = sqlite3_value_blob (argv[1]);
	  blob_sz = sqlite3_value_bytes (argv[1]);
	  if (!get_coverage_sample_bands
	      (sqlite, coverage, &sample_type, &num_bands))
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret =
	rl2_is_valid_dbms_raster_statistics (blob, blob_sz, sample_type,
					     num_bands);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static int
get_coverage_defs (sqlite3 * sqlite, const char *coverage,
		   unsigned short *tile_width, unsigned short *tile_height,
		   unsigned char *sample_type, unsigned char *pixel_type,
		   unsigned char *num_bands, unsigned char *compression)
{
/* attempting to retrieve the main definitions from a Coverage */
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    int ret;
    const char *sample;
    const char *pixel;
    const char *compr;
    int bands;
    unsigned char xsample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char xpixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char xnum_bands = RL2_BANDS_UNKNOWN;
    unsigned char xcompression = RL2_COMPRESSION_UNKNOWN;
    unsigned short xtile_width = RL2_TILESIZE_UNDEFINED;
    unsigned short xtile_height = RL2_TILESIZE_UNDEFINED;

    sql = sqlite3_mprintf ("SELECT sample_type, pixel_type, num_bands, "
			   "compression, tile_width, tile_height FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		sample = results[(i * columns) + 0];
		if (strcmp (sample, "1-BIT") == 0)
		    xsample_type = RL2_SAMPLE_1_BIT;
		if (strcmp (sample, "2-BIT") == 0)
		    xsample_type = RL2_SAMPLE_2_BIT;
		if (strcmp (sample, "4-BIT") == 0)
		    xsample_type = RL2_SAMPLE_4_BIT;
		if (strcmp (sample, "INT8") == 0)
		    xsample_type = RL2_SAMPLE_INT8;
		if (strcmp (sample, "UINT8") == 0)
		    xsample_type = RL2_SAMPLE_UINT8;
		if (strcmp (sample, "INT16") == 0)
		    xsample_type = RL2_SAMPLE_INT16;
		if (strcmp (sample, "UINT16") == 0)
		    xsample_type = RL2_SAMPLE_UINT16;
		if (strcmp (sample, "INT32") == 0)
		    xsample_type = RL2_SAMPLE_INT32;
		if (strcmp (sample, "UINT32") == 0)
		    xsample_type = RL2_SAMPLE_UINT32;
		if (strcmp (sample, "FLOAT") == 0)
		    xsample_type = RL2_SAMPLE_FLOAT;
		if (strcmp (sample, "DOUBLE") == 0)
		    xsample_type = RL2_SAMPLE_DOUBLE;
		pixel = results[(i * columns) + 1];
		if (strcmp (pixel, "MONOCHROME") == 0)
		    xpixel_type = RL2_PIXEL_MONOCHROME;
		if (strcmp (pixel, "PALETTE") == 0)
		    xpixel_type = RL2_PIXEL_PALETTE;
		if (strcmp (pixel, "GRAYSCALE") == 0)
		    xpixel_type = RL2_PIXEL_GRAYSCALE;
		if (strcmp (pixel, "RGB") == 0)
		    xpixel_type = RL2_PIXEL_RGB;
		if (strcmp (pixel, "MULTIBAND") == 0)
		    xpixel_type = RL2_PIXEL_MULTIBAND;
		if (strcmp (pixel, "DATAGRID") == 0)
		    xpixel_type = RL2_PIXEL_DATAGRID;
		bands = atoi (results[(i * columns) + 2]);
		if (bands > 0 && bands < 256)
		    xnum_bands = bands;
		compr = results[(i * columns) + 3];
		if (strcmp (compr, "NONE") == 0)
		    xcompression = RL2_COMPRESSION_NONE;
		if (strcmp (compr, "DEFLATE") == 0)
		    xcompression = RL2_COMPRESSION_DEFLATE;
		if (strcmp (compr, "LZMA") == 0)
		    xcompression = RL2_COMPRESSION_LZMA;
		if (strcmp (compr, "GIF") == 0)
		    xcompression = RL2_COMPRESSION_GIF;
		if (strcmp (compr, "PNG") == 0)
		    xcompression = RL2_COMPRESSION_PNG;
		if (strcmp (compr, "JPEG") == 0)
		    xcompression = RL2_COMPRESSION_JPEG;
		if (strcmp (compr, "LOSSY_WEBP") == 0)
		    xcompression = RL2_COMPRESSION_LOSSY_WEBP;
		if (strcmp (compr, "LOSSLESS_WEBP") == 0)
		    xcompression = RL2_COMPRESSION_LOSSLESS_WEBP;
		if (strcmp (compr, "CCITTFAX3") == 0)
		    xcompression = RL2_COMPRESSION_CCITTFAX3;
		if (strcmp (compr, "CCITTFAX4") == 0)
		    xcompression = RL2_COMPRESSION_CCITTFAX4;
		xtile_width = atoi (results[(i * columns) + 4]);
		xtile_height = atoi (results[(i * columns) + 5]);
	    }
      }
    sqlite3_free_table (results);
    if (xsample_type == RL2_SAMPLE_UNKNOWN || xpixel_type == RL2_PIXEL_UNKNOWN
	|| xnum_bands == RL2_BANDS_UNKNOWN
	|| xcompression == RL2_COMPRESSION_UNKNOWN
	|| xtile_width == RL2_TILESIZE_UNDEFINED
	|| xtile_height == RL2_TILESIZE_UNDEFINED)
	return 0;
    *sample_type = xsample_type;
    *pixel_type = xpixel_type;
    *num_bands = xnum_bands;
    *compression = xcompression;
    *tile_width = xtile_width;
    *tile_height = xtile_height;
    return 1;
}

static void
fnct_IsValidRasterTile (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterTile(text coverage, integer level, BLOBencoded tile_odd,
/   BLOBencoded tile_even)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    int level;
    const unsigned char *blob_odd;
    int blob_odd_sz;
    const unsigned char *blob_even;
    int blob_even_sz;
    const char *coverage;
    unsigned short tile_width;
    unsigned short tile_height;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    unsigned char compression = RL2_COMPRESSION_UNKNOWN;
    sqlite3 *sqlite;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_BLOB
	&& sqlite3_value_type (argv[3]) != SQLITE_NULL)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    level = sqlite3_value_int (argv[1]);
    blob_odd = sqlite3_value_blob (argv[2]);
    blob_odd_sz = sqlite3_value_bytes (argv[2]);
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
      {
	  blob_even = NULL;
	  blob_even_sz = 0;
      }
    else
      {
	  blob_even = sqlite3_value_blob (argv[3]);
	  blob_even_sz = sqlite3_value_bytes (argv[3]);
      }
    if (!get_coverage_defs
	(sqlite, coverage, &tile_width, &tile_height, &sample_type, &pixel_type,
	 &num_bands, &compression))
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    ret =
	rl2_is_valid_dbms_raster_tile (level, tile_width, tile_height, blob_odd,
				       blob_odd_sz, blob_even, blob_even_sz,
				       sample_type, pixel_type, num_bands,
				       compression);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_CreatePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ CreatePixel(text sample_type, text pixel_type, int num_bands)
/
/ will return a serialized binary Pixel Object 
/ or NULL on failure
*/
    int err = 0;
    const char *sample_type;
    const char *pixel_type;
    int num_bands;
    unsigned char sample;
    unsigned char pixel;
    rl2PixelPtr pxl = NULL;
    unsigned char *blob = NULL;
    int blob_sz = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto error;

/* retrieving the arguments */
    sample_type = (const char *) sqlite3_value_text (argv[0]);
    pixel_type = (const char *) sqlite3_value_text (argv[1]);
    num_bands = sqlite3_value_int (argv[2]);

/* preliminary arg checking */
    if (num_bands < 1 || num_bands > 255)
	goto error;

    sample = RL2_SAMPLE_UNKNOWN;
    if (strcasecmp (sample_type, "1-BIT") == 0)
	sample = RL2_SAMPLE_1_BIT;
    if (strcasecmp (sample_type, "2-BIT") == 0)
	sample = RL2_SAMPLE_2_BIT;
    if (strcasecmp (sample_type, "4-BIT") == 0)
	sample = RL2_SAMPLE_4_BIT;
    if (strcasecmp (sample_type, "INT8") == 0)
	sample = RL2_SAMPLE_INT8;
    if (strcasecmp (sample_type, "UINT8") == 0)
	sample = RL2_SAMPLE_UINT8;
    if (strcasecmp (sample_type, "INT16") == 0)
	sample = RL2_SAMPLE_INT16;
    if (strcasecmp (sample_type, "UINT16") == 0)
	sample = RL2_SAMPLE_UINT16;
    if (strcasecmp (sample_type, "INT32") == 0)
	sample = RL2_SAMPLE_INT32;
    if (strcasecmp (sample_type, "UINT32") == 0)
	sample = RL2_SAMPLE_UINT32;
    if (strcasecmp (sample_type, "FLOAT") == 0)
	sample = RL2_SAMPLE_FLOAT;
    if (strcasecmp (sample_type, "DOUBLE") == 0)
	sample = RL2_SAMPLE_DOUBLE;

    pixel = RL2_PIXEL_UNKNOWN;
    if (strcasecmp (pixel_type, "MONOCHROME") == 0)
	pixel = RL2_PIXEL_MONOCHROME;
    if (strcasecmp (pixel_type, "GRAYSCALE") == 0)
	pixel = RL2_PIXEL_GRAYSCALE;
    if (strcasecmp (pixel_type, "PALETTE") == 0)
	pixel = RL2_PIXEL_PALETTE;
    if (strcasecmp (pixel_type, "RGB") == 0)
	pixel = RL2_PIXEL_RGB;
    if (strcasecmp (pixel_type, "DATAGRID") == 0)
	pixel = RL2_PIXEL_DATAGRID;
    if (strcasecmp (pixel_type, "MULTIBAND") == 0)
	pixel = RL2_PIXEL_MULTIBAND;

/* attempting to create the Pixel */
    pxl = rl2_create_pixel (sample, pixel, (unsigned char) num_bands);
    if (pxl == NULL)
	goto error;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelType (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelType(BLOB pixel_obj)
/
/ will return one of "1-BIT", "2-BIT", "4-BIT", "INT8", "UINT8", "INT16",
/      "UINT16", "INT32", "UINT32", "FLOAT", "DOUBLE", "UNKNOWN" 
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    const char *text;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
	  text = "1-BIT";
	  break;
      case RL2_SAMPLE_2_BIT:
	  text = "2-BIT";
	  break;
      case RL2_SAMPLE_4_BIT:
	  text = "4-BIT";
	  break;
      case RL2_SAMPLE_INT8:
	  text = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  text = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  text = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  text = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  text = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  text = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  text = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  text = "DOUBLE";
	  break;
      default:
	  text = "UNKNOWN";
	  break;
      };
    sqlite3_result_text (context, text, strlen (text), SQLITE_TRANSIENT);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelSampleType (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelSampleType(BLOB pixel_obj)
/
/ will return one of "MONOCHROME" "PALETTE", "GRAYSCALE", "RGB",
/      "DATAGRID", "MULTIBAND", "UNKNOWN" 
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    const char *text;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    switch (pixel->pixelType)
      {
      case RL2_PIXEL_MONOCHROME:
	  text = "MONOCHROME";
	  break;
      case RL2_PIXEL_PALETTE:
	  text = "PALETTE";
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  text = "GRAYSCALE";
	  break;
      case RL2_PIXEL_RGB:
	  text = "RGB";
	  break;
      case RL2_PIXEL_DATAGRID:
	  text = "DATAGRID";
	  break;
      case RL2_PIXEL_MULTIBAND:
	  text = "MULTIBAND";
	  break;
      default:
	  text = "UNKNOWN";
	  break;
      };
    sqlite3_result_text (context, text, strlen (text), SQLITE_TRANSIENT);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelNumBands (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelNumBands(BLOB pixel_obj)
/
/ will return the number of Bands
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    sqlite3_result_int (context, pixel->nBands);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelValue (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelValue(BLOB pixel_obj, INT band)
/
/ will return the corresponding Band value
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    rl2PrivSamplePtr sample;
    int band_id;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_id = sqlite3_value_int (argv[1]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (band_id >= 0 && band_id < pixel->nBands)
	;
    else
	goto error;
    sample = pixel->Samples + band_id;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  sqlite3_result_int (context, sample->int8);
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  sqlite3_result_int (context, sample->uint8);
	  break;
      case RL2_SAMPLE_INT16:
	  sqlite3_result_int (context, sample->int16);
	  break;
      case RL2_SAMPLE_UINT16:
	  sqlite3_result_int (context, sample->uint16);
	  break;
      case RL2_SAMPLE_INT32:
	  sqlite3_result_int64 (context, sample->int32);
	  break;
      case RL2_SAMPLE_UINT32:
	  sqlite3_result_int64 (context, sample->uint32);
	  break;
      case RL2_SAMPLE_FLOAT:
	  sqlite3_result_double (context, sample->float32);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sqlite3_result_double (context, sample->float64);
	  break;
      default:
	  goto error;
      };
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetPixelValue (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ SetPixelValue(BLOB pixel_obj, INT band, INT value)
/ SetPixelValue(BLOB pixel_obj, INT band, FLOAT value)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    rl2PrivSamplePtr sample;
    int band_id;
    int intval;
    double dblval;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[2]) != SQLITE_FLOAT)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_id = sqlite3_value_int (argv[1]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (band_id >= 0 && band_id < pixel->nBands)
	;
    else
	goto error;
    if (pixel->sampleType == RL2_SAMPLE_FLOAT
	|| pixel->sampleType == RL2_SAMPLE_DOUBLE)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      dblval = sqlite3_value_double (argv[2]);
	  else
	    {
		intval = sqlite3_value_int (argv[2]);
		dblval = intval;
	    }
      }
    else
      {
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      goto error;
	  intval = sqlite3_value_int (argv[2]);
      }
    sample = pixel->Samples + band_id;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  if (intval < INT8_MIN || intval > INT8_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_1_BIT:
	  if (intval < 0 || intval > 1)
	      goto error;
	  break;
      case RL2_SAMPLE_2_BIT:
	  if (intval < 0 || intval > 3)
	      goto error;
	  break;
      case RL2_SAMPLE_4_BIT:
	  if (intval < 0 || intval > 15)
	      goto error;
	  break;
      case RL2_SAMPLE_UINT8:
	  if (intval < 0 || intval > UINT8_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_INT16:
	  if (intval < INT16_MIN || intval > INT16_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_UINT16:
	  if (intval < 0 || intval > UINT16_MAX)
	      goto error;
	  break;
      };
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  sample->int8 = intval;
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  sample->uint8 = intval;
	  break;
      case RL2_SAMPLE_INT16:
	  sample->int16 = intval;
	  break;
      case RL2_SAMPLE_UINT16:
	  sample->uint16 = intval;
	  break;
      case RL2_SAMPLE_INT32:
	  sample->int32 = intval;
	  break;
      case RL2_SAMPLE_UINT32:
	  sample->uint32 = (unsigned int) intval;
	  break;
      case RL2_SAMPLE_FLOAT:
	  sample->float32 = dblval;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample->float64 = dblval;
	  break;
      default:
	  goto error;
      };
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_IsTransparentPixel (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ IsTransparentPixel(BLOB pixel_obj)
/
/ 1 (TRUE) or 0 (FALSE)
/ or NULL on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (pixel->isTransparent == 0)
	sqlite3_result_int (context, 0);
    else
	sqlite3_result_int (context, 1);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_IsOpaquePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsOpaquePixel(BLOB pixel_obj)
/
/ 1 (TRUE) or 0 (FALSE)
/ or NULL on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (pixel->isTransparent == 0)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetTransparentPixel (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ SetTransparentPixel(BLOB pixel_obj)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    pixel->isTransparent = 1;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetOpaquePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ SetOpaquePixel(BLOB pixel_obj)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    pixel->isTransparent = 0;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_PixelEquals (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ PixelEquals(BLOB pixel_obj1, BLOB pixel_obj2)
/
/ 1 (TRUE) or 0 (FALSE)
/ or NULL on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl1 = NULL;
    rl2PixelPtr pxl2 = NULL;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl1 = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl1 == NULL)
	goto error;
    blob = sqlite3_value_blob (argv[1]);
    blob_sz = sqlite3_value_bytes (argv[1]);
    pxl2 = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl2 == NULL)
	goto error;
    ret = rl2_compare_pixels (pxl1, pxl2);
    if (ret == RL2_TRUE)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    rl2_destroy_pixel (pxl1);
    rl2_destroy_pixel (pxl2);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl1 != NULL)
	rl2_destroy_pixel (pxl1);
    if (pxl2 != NULL)
	rl2_destroy_pixel (pxl2);
}

static void
fnct_CreateCoverage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double res)
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double horz_res, double vert_res)
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double horz_res, double vert_res, BLOB no_data)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    const char *sample_type;
    const char *pixel_type;
    int num_bands;
    const char *compression;
    int quality;
    int tile_width;
    int tile_height;
    int srid;
    double horz_res;
    double vert_res;
    unsigned char sample;
    unsigned char pixel;
    unsigned char compr;
    sqlite3 *sqlite;
    int ret;
    rl2PixelPtr no_data = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 10)
      {
	  if (sqlite3_value_type (argv[10]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[10]) != SQLITE_FLOAT)
	      err = 1;
      }
    if (argc > 11)
      {
	  if (sqlite3_value_type (argv[11]) != SQLITE_BLOB
	      && sqlite3_value_type (argv[11]) != SQLITE_NULL)
	      err = 1;
      }
    if (err)
	goto error;

/* retrieving the arguments */
    coverage = (const char *) sqlite3_value_text (argv[0]);
    sample_type = (const char *) sqlite3_value_text (argv[1]);
    pixel_type = (const char *) sqlite3_value_text (argv[2]);
    num_bands = sqlite3_value_int (argv[3]);
    compression = (const char *) sqlite3_value_text (argv[4]);
    quality = sqlite3_value_int (argv[5]);
    tile_width = sqlite3_value_int (argv[6]);
    tile_height = sqlite3_value_int (argv[7]);
    srid = sqlite3_value_int (argv[8]);
    if (sqlite3_value_type (argv[9]) == SQLITE_FLOAT)
	horz_res = sqlite3_value_double (argv[9]);
    else
      {
	  int val = sqlite3_value_int (argv[9]);
	  horz_res = val;
      }
    if (argc > 10)
      {
	  if (sqlite3_value_type (argv[10]) == SQLITE_FLOAT)
	      vert_res = sqlite3_value_double (argv[10]);
	  else
	    {
		int val = sqlite3_value_int (argv[10]);
		vert_res = val;
	    }
      }
    else
	vert_res = horz_res;
    if (argc > 11 && sqlite3_value_type (argv[11]) == SQLITE_BLOB)
      {
	  /* NO-DATA pixel */
	  const unsigned char *blob = sqlite3_value_blob (argv[11]);
	  int blob_sz = sqlite3_value_bytes (argv[11]);
	  no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
	  if (no_data == NULL)
	      goto error;
      }

/* preliminary arg checking */
    if (num_bands < 1 || num_bands > 255)
	goto error;
    if (quality < 0)
	quality = 0;
    if (quality > 100)
	quality = 100;
    if (tile_width < 0 || tile_width > 65536)
	goto error;
    if (tile_height < 0 || tile_height > 65536)
	goto error;

    sample = RL2_SAMPLE_UNKNOWN;
    if (strcasecmp (sample_type, "1-BIT") == 0)
	sample = RL2_SAMPLE_1_BIT;
    if (strcasecmp (sample_type, "2-BIT") == 0)
	sample = RL2_SAMPLE_2_BIT;
    if (strcasecmp (sample_type, "4-BIT") == 0)
	sample = RL2_SAMPLE_4_BIT;
    if (strcasecmp (sample_type, "INT8") == 0)
	sample = RL2_SAMPLE_INT8;
    if (strcasecmp (sample_type, "UINT8") == 0)
	sample = RL2_SAMPLE_UINT8;
    if (strcasecmp (sample_type, "INT16") == 0)
	sample = RL2_SAMPLE_INT16;
    if (strcasecmp (sample_type, "UINT16") == 0)
	sample = RL2_SAMPLE_UINT16;
    if (strcasecmp (sample_type, "INT32") == 0)
	sample = RL2_SAMPLE_INT32;
    if (strcasecmp (sample_type, "UINT32") == 0)
	sample = RL2_SAMPLE_UINT32;
    if (strcasecmp (sample_type, "FLOAT") == 0)
	sample = RL2_SAMPLE_FLOAT;
    if (strcasecmp (sample_type, "DOUBLE") == 0)
	sample = RL2_SAMPLE_DOUBLE;

    pixel = RL2_PIXEL_UNKNOWN;
    if (strcasecmp (pixel_type, "MONOCHROME") == 0)
	pixel = RL2_PIXEL_MONOCHROME;
    if (strcasecmp (pixel_type, "GRAYSCALE") == 0)
	pixel = RL2_PIXEL_GRAYSCALE;
    if (strcasecmp (pixel_type, "PALETTE") == 0)
	pixel = RL2_PIXEL_PALETTE;
    if (strcasecmp (pixel_type, "RGB") == 0)
	pixel = RL2_PIXEL_RGB;
    if (strcasecmp (pixel_type, "DATAGRID") == 0)
	pixel = RL2_PIXEL_DATAGRID;
    if (strcasecmp (pixel_type, "MULTIBAND") == 0)
	pixel = RL2_PIXEL_MULTIBAND;

    compr = RL2_COMPRESSION_NONE;
    if (strcasecmp (compression, "NONE") == 0)
	compr = RL2_COMPRESSION_NONE;
    if (strcasecmp (compression, "DEFLATE") == 0)
	compr = RL2_COMPRESSION_DEFLATE;
    if (strcasecmp (compression, "LZMA") == 0)
	compr = RL2_COMPRESSION_LZMA;
    if (strcasecmp (compression, "PNG") == 0)
	compr = RL2_COMPRESSION_PNG;
    if (strcasecmp (compression, "GIF") == 0)
	compr = RL2_COMPRESSION_GIF;
    if (strcasecmp (compression, "JPEG") == 0)
	compr = RL2_COMPRESSION_JPEG;
    if (strcasecmp (compression, "LOSSY_WEBP") == 0)
	compr = RL2_COMPRESSION_LOSSY_WEBP;
    if (strcasecmp (compression, "LOSSLESS_WEBP") == 0)
	compr = RL2_COMPRESSION_LOSSLESS_WEBP;
    if (strcasecmp (compression, "FAX3") == 0)
	compr = RL2_COMPRESSION_CCITTFAX3;
    if (strcasecmp (compression, "FAX4") == 0)
	compr = RL2_COMPRESSION_CCITTFAX4;

/* attempting to create the DBMS Coverage */
    sqlite = sqlite3_context_db_handle (context);
    ret = rl2_create_dbms_coverage (sqlite, coverage, sample, pixel,
				    (unsigned char) num_bands,
				    compr, quality,
				    (unsigned short) tile_width,
				    (unsigned short) tile_height, srid,
				    horz_res, vert_res, no_data);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    return;

  error:
    sqlite3_result_int (context, -1);
}

static void
fnct_LoadRaster (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ LoadRaster(text coverage, text path_to_raster)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int worldfile = 0;
    int force_srid = -1;
    int transaction = 0;
    rl2CoveragePtr coverage;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    rl2_destroy_coverage (coverage);
/* attempting to load the Raster into the DBMS */
    path = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	worldfile = sqlite3_value_int (argv[2]);
    if (argc > 3)
	force_srid = sqlite3_value_int (argv[3]);
    if (argc > 4)
	transaction = sqlite3_value_int (argv[4]);
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret = rl2_load_raster_into_dbms (sqlite, path, coverage,
				     worldfile, force_srid, 0);
    rl2_destroy_coverage (coverage);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_LoadRastersFromDir (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ LoadRastersFromDir(text coverage, text dir_path)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid, 
/                    int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    const char *file_ext;
    int worldfile = 0;
    int force_srid = -1;
    int transaction = 1;
    rl2CoveragePtr coverage;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
/* attempting to load the Rasters into the DBMS */
    path = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	file_ext = (const char *) sqlite3_value_text (argv[2]);
    if (argc > 3)
	worldfile = sqlite3_value_int (argv[2]);
    if (argc > 4)
	force_srid = sqlite3_value_int (argv[3]);
    if (argc > 5)
	transaction = sqlite3_value_int (argv[5]);
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret = rl2_load_mrasters_into_dbms (sqlite, path, file_ext, coverage,
				       worldfile, force_srid, 0);
    rl2_destroy_coverage (coverage);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
register_rl2_sql_functions (void *p_db)
{
    sqlite3 *db = p_db;
    const char *security_level;
    sqlite3_create_function (db, "rl2_version", 0, SQLITE_ANY, 0,
			     fnct_rl2_version, 0, 0);
    sqlite3_create_function (db, "rl2_target_cpu", 0, SQLITE_ANY, 0,
			     fnct_rl2_target_cpu, 0, 0);
    sqlite3_create_function (db, "IsValidPixel", 3, SQLITE_ANY, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidPixel", 3, SQLITE_ANY, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "IsValidRasterPalette", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterPalette", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 2, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterStatistics", 2, SQLITE_ANY,
			     0, fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 3, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterStatistics", 3, SQLITE_ANY,
			     0, fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterTile", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterTile", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 10, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 11, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 12, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 10, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 11, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 12, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "CreatePixel", 3, SQLITE_ANY, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "RL2_CreatePixel", 3, SQLITE_ANY, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "GetPixelType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "GetPixelSampleType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelSampleType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "GetPixelNumBands", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelNumBands", 1, SQLITE_ANY, 0,
			     fnct_GetPixelNumBands, 0, 0);
    sqlite3_create_function (db, "GetPixelValue", 2, SQLITE_ANY, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelValue", 2, SQLITE_ANY, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "SetPixelValue", 3, SQLITE_ANY, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_SetPixelValue", 3, SQLITE_ANY, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "IsTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "IsOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "SetTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "SetOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "PixelEquals", 2, SQLITE_ANY, 0,
			     fnct_PixelEquals, 0, 0);
    sqlite3_create_function (db, "RL2_PixelEquals", 2, SQLITE_ANY, 0,
			     fnct_PixelEquals, 0, 0);

/*
// enabling ImportRaster and ExportRaster
//
// these functions could potentially introduce serious security issues,
// most notably when invoked from within some Trigger
// - BlobToFile: some arbitrary code, possibly harmful (e.g. virus or 
//   trojan) could be installed on the local file-system, the user being
//   completely unaware of this
// - ImportRaster: some file could be maliciously "stolen" from the local
//   file system and then inserted into the DB
// - ExportRaster could eventually install some malicious payload into the
//   local filesystem; or could potentially flood the local file-system by
//   outputting a huge size of data
//
// so by default such functions are disabled.
// if for any good/legitimate reason the user really wants to enable them
// the following environment variable has to be explicitly declared:
//
// SPATIALITE_SECURITY=relaxed
//
*/
    security_level = getenv ("SPATIALITE_SECURITY");
    if (security_level == NULL)
	;
    else if (strcasecmp (security_level, "relaxed") == 0)
      {
	  sqlite3_create_function (db, "LoadRaster", 2, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 3, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 4, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 5, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 2, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 3, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 4, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 5, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 2, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 3, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 4, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 5, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 6, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 2, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 3, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 4, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 5, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 6, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
      }
}

static void
rl2_splash_screen (int verbose)
{
    if (isatty (1))
      {
	  /* printing "hello" message only when stdout is on console */
	  if (verbose)
	    {
		printf ("RasterLite-2 version : %s\n", rl2_version ());
		printf ("TARGET CPU ..........: %s\n", rl2_target_cpu ());
	    }
      }
}

#ifndef LOADABLE_EXTENSION

RL2_DECLARE void
rl2_init (sqlite3 * handle, int verbose)
{
/* used when SQLite initializes as an ordinary lib */
    register_rl2_sql_functions (handle);
    rl2_splash_screen (verbose);
}

#else /* built as LOADABLE EXTENSION only */

SQLITE_EXTENSION_INIT1 static int
init_rl2_extension (sqlite3 * db, char **pzErrMsg,
		    const sqlite3_api_routines * pApi)
{
    SQLITE_EXTENSION_INIT2 (pApi);

    register_rl2_sql_functions (db);
    return 0;
}

#if !(defined _WIN32) || defined(__MINGW32__)
/* MSVC is unable to understand this declaration */
__attribute__ ((visibility ("default")))
#endif
     RL2_DECLARE int
	 sqlite3_rasterlite_init (sqlite3 * db, char **pzErrMsg,
				  const sqlite3_api_routines * pApi)
{
/* SQLite invokes this routine once when it dynamically loads the extension. */
    return init_rl2_extension (db, pzErrMsg, pApi);
}

#endif
