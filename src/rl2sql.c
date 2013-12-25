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

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"

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
fnct_IsValidNoDataPixel (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ IsValidNoDataPixel(BLOBencoded pixel, text sample_type, int num_bands)
/
/ will return 1 (TRUE, valid) or (FALSE, invalid)
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
	  ret =
	      rl2_is_valid_dbms_no_data (blob, blob_sz, sample_type, num_bands);
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
/ will return 1 (TRUE, valid) or (FALSE, invalid)
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
/ will return 1 (TRUE, valid) or (FALSE, invalid)
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
/ will return 1 (TRUE, valid) or (FALSE, invalid)
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
register_rl2_sql_functions (void *p_db)
{
    sqlite3 *db = p_db;
    sqlite3_create_function (db, "rl2_version", 0, SQLITE_ANY, 0,
			     fnct_rl2_version, 0, 0);
    sqlite3_create_function (db, "rl2_target_cpu", 0, SQLITE_ANY, 0,
			     fnct_rl2_target_cpu, 0, 0);
    sqlite3_create_function (db, "IsValidNoDataPixel", 3, SQLITE_ANY, 0,
			     fnct_IsValidNoDataPixel, 0, 0);
    sqlite3_create_function (db, "IsValidRasterPalette", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 2, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 3, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterTile", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterTile, 0, 0);
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
