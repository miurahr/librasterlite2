/*

 test_map_infrared.c -- RasterLite-2 Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the RasterLite2 library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2013
the Initial Developer. All Rights Reserved.

Contributor(s):

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"
#include "spatialite/gaiaaux.h"

#include "rasterlite2/rasterlite2.h"

#define TILE_256	256
#define TILE_512	512
#define TILE_1024	1024

static int
execute_check (sqlite3 * sqlite, const char *sql)
{
/* executing an SQL statement returning True/False */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    if (retcode == 1)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
execute_check_boolean (sqlite3 * sqlite, const char *sql)
{
/* executing an SQL statement returning True/False */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = -1;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return -1;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
	    {
		int value = sqlite3_column_int (stmt, 0);
		if (value < 0)
		    retcode = -1;
		else if (value == 0)
		    retcode = 0;
		else
		    retcode = 1;
	    }
      }
    sqlite3_finalize (stmt);
    if (retcode >= 0)
	return retcode;
    return -1;
}

static int
execute_check_value (sqlite3 * sqlite, const char *sql, int *value)
{
/* executing an SQL statement returning an Integer value */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return -1;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
	    {
		*value = sqlite3_column_int (stmt, 0);
		retcode = 1;
	    }
      }
    sqlite3_finalize (stmt);
    return retcode;
}

static int
get_max_tile_id (sqlite3 * sqlite, const char *coverage)
{
/* retriving the Max tile_id for a given Coverage */
    char *sql;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt;
    int ret;
    int max = 0;

    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Max(tile_id) FROM \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      max = sqlite3_column_int (stmt, 0);
      }
    sqlite3_finalize (stmt);
    return max;
}

static int
do_export_tile_image (sqlite3 * sqlite, const char *coverage, int tile_id)
{
/* attempting to export a visible Tile - basic */
    char *sql;
    char *path;
    int ret;
    int transparent = 1;

    if (tile_id > 1)
	transparent = 0;
    if (tile_id < 0)
	tile_id = get_max_tile_id (sqlite, coverage);
    path = sqlite3_mprintf ("./%s_tile_%d.png", coverage, tile_id);
    sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetTileImage('main', %Q, %d, '#e0ffe0', %d), %Q)",
	 coverage, tile_id, transparent, path);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    unlink (path);
    sqlite3_free (path);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "ERROR: Unable to export an Image from \"%s\" tile_id=%d\n",
		   coverage, tile_id);
	  return 0;
      }
    return 1;
}

static int
do_export_tile_image3 (sqlite3 * sqlite, const char *coverage, int tile_id,
		       int band_mix)
{
/* attempting to export a visible Tile - triple band */
    char *sql;
    char *path;
    int ret;
    int transparent = 1;
    unsigned char red_band = 0;
    unsigned char green_band = 1;
    unsigned char blue_band = 2;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 2;
      }
    if (band_mix == 2)
      {
	  red_band = 0;
	  green_band = 1;
	  blue_band = 3;
      }
    if (tile_id <= 1)
	transparent = 0;
    if (tile_id < 0)
	tile_id = get_max_tile_id (sqlite, coverage);
    path = sqlite3_mprintf ("./%s_tile_%d_%d.png", coverage, tile_id, band_mix);
    sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetTripleBandTileImage('main', %Q, %d, %d, %d, %d, '#e0ffe0', %d), %Q)",
	 coverage, tile_id, red_band, green_band, blue_band, transparent, path);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    unlink (path);
    sqlite3_free (path);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "ERROR: Unable to export an Image from \"%s\" tile_id=%d\n",
		   coverage, tile_id);
	  return 0;
      }
    return 1;
}

static int
do_export_mono_tile_image (sqlite3 * sqlite, const char *coverage, int tile_id,
			   int band_mix)
{
/* attempting to export a visible Tile (Mono-Band) */
    char *sql;
    char *path;
    int ret;
    int transparent = 1;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 2;
    if (band_mix == 2)
	mono_band = 3;
    if (tile_id <= 1)
	transparent = 0;
    if (tile_id < 0)
	tile_id = get_max_tile_id (sqlite, coverage);
    path =
	sqlite3_mprintf ("./%s_mono_tile_%d_%d.png", coverage, tile_id,
			 band_mix);
    sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetMonoBandTileImage('main', %Q, %d, %d, '#e0ffe0', %d), %Q)",
	 coverage, tile_id, mono_band, transparent, path);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    unlink (path);
    sqlite3_free (path);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "ERROR: Unable to export an Image from \"%s\" tile_id=%d\n",
		   coverage, tile_id);
	  return 0;
      }
    return 1;
}

static int
get_base_resolution (sqlite3 * sqlite, const char *coverage, double *x_res,
		     double *y_res)
{
/* attempting to retrieve the Coverage's base resolution */
    char *sql;
    sqlite3_stmt *stmt;
    int ret;
    int ok = 0;

    sql = sqlite3_mprintf ("SELECT horz_resolution, vert_resolution "
			   "FROM raster_coverages WHERE coverage_name = %Q",
			   coverage);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*x_res = sqlite3_column_double (stmt, 0);
		*y_res = sqlite3_column_double (stmt, 1);
		ok = 1;
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static int
do_export_geotiff (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		   int scale)
{
/* exporting a GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;

    path = sqlite3_mprintf ("./%s_gt_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteGeoTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 5, blob, blob_size, free);
    sqlite3_bind_double (stmt, 6, xx_res);
    sqlite3_bind_double (stmt, 7, yy_res);
    sqlite3_bind_int (stmt, 8, 0);
    sqlite3_bind_text (stmt, 9, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_band_composed_geotiff (sqlite3 * sqlite, const char *coverage,
				 gaiaGeomCollPtr geom, int scale, int band_mix,
				 int with_worldfile)
{
/* exporting a GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 0;
    unsigned char green_band = 1;
    unsigned char blue_band = 2;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 2;
      }
    if (band_mix == 2)
      {
	  red_band = 0;
	  green_band = 1;
	  blue_band = 3;
      }

    path = sqlite3_mprintf ("./%s_bc_gt_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteTripleBandGeoTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, red_band);
    sqlite3_bind_int (stmt, 6, green_band);
    sqlite3_bind_int (stmt, 7, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, free);
    sqlite3_bind_double (stmt, 9, xx_res);
    sqlite3_bind_double (stmt, 10, yy_res);
    sqlite3_bind_int (stmt, 11, with_worldfile);
    sqlite3_bind_text (stmt, 12, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path = sqlite3_mprintf ("./%s_bc_gt_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_band_composed_tiff (sqlite3 * sqlite, const char *coverage,
			      gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a plain Tiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 0;
    unsigned char green_band = 1;
    unsigned char blue_band = 2;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 2;
      }
    if (band_mix == 2)
      {
	  red_band = 0;
	  green_band = 1;
	  blue_band = 3;
      }

    path = sqlite3_mprintf ("./%s_bc_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteTripleBandTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, red_band);
    sqlite3_bind_int (stmt, 6, green_band);
    sqlite3_bind_int (stmt, 7, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, free);
    sqlite3_bind_double (stmt, 9, xx_res);
    sqlite3_bind_double (stmt, 10, yy_res);
    sqlite3_bind_text (stmt, 11, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_band_composed_tiff_tfw (sqlite3 * sqlite, const char *coverage,
				  gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a Tiff+TFW */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 0;
    unsigned char green_band = 1;
    unsigned char blue_band = 2;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 2;
      }
    if (band_mix == 2)
      {
	  red_band = 0;
	  green_band = 1;
	  blue_band = 3;
      }

    path = sqlite3_mprintf ("./%s_bc_tfw_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteTripleBandTiffTfw('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, red_band);
    sqlite3_bind_int (stmt, 6, green_band);
    sqlite3_bind_int (stmt, 7, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, free);
    sqlite3_bind_double (stmt, 9, xx_res);
    sqlite3_bind_double (stmt, 10, yy_res);
    sqlite3_bind_text (stmt, 11, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path = sqlite3_mprintf ("./%s_bc_tfw_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_mono_band_geotiff (sqlite3 * sqlite, const char *coverage,
			     gaiaGeomCollPtr geom, int scale, int band_mix,
			     int with_worldfile)
{
/* exporting a GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 1;
    if (band_mix == 2)
	mono_band = 3;

    path =
	sqlite3_mprintf ("./%s_mono_gt_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteMonoBandGeoTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, mono_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_int (stmt, 9, with_worldfile);
    sqlite3_bind_text (stmt, 10, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path =
	sqlite3_mprintf ("./%s_mono_gt_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_mono_band_tiff (sqlite3 * sqlite, const char *coverage,
			  gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a plain Tiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 1;
    if (band_mix == 2)
	mono_band = 3;

    path = sqlite3_mprintf ("./%s_mono_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteMonoBandTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, mono_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_text (stmt, 9, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_mono_band_tiff_tfw (sqlite3 * sqlite, const char *coverage,
			      gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a Tiff+TFW */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 1;
    if (band_mix == 2)
	mono_band = 3;

    path =
	sqlite3_mprintf ("./%s_mono_tfw_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteMonoBandTiffTfw('main', ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, mono_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_text (stmt, 9, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path =
	sqlite3_mprintf ("./%s_mono_tfw_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_triple_geotiff (sqlite3 * sqlite, const char *coverage,
				  gaiaGeomCollPtr geom, int scale)
{
/* exporting a Section TripleBand GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 0;
    unsigned char green_band = 1;
    unsigned char blue_band = 2;

    path = sqlite3_mprintf ("./%s_sect1_triple_gt_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteSectionTripleBandGeoTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    sqlite3_bind_int (stmt, 6, red_band);
    sqlite3_bind_int (stmt, 7, green_band);
    sqlite3_bind_int (stmt, 8, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 9, blob, blob_size, free);
    sqlite3_bind_double (stmt, 10, xx_res);
    sqlite3_bind_double (stmt, 11, yy_res);
    sqlite3_bind_int (stmt, 12, 0);
    sqlite3_bind_text (stmt, 13, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_triple_tiff (sqlite3 * sqlite, const char *coverage,
			       gaiaGeomCollPtr geom, int scale)
{
/* exporting a Section TripleBand Tiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 0;
    unsigned char green_band = 1;
    unsigned char blue_band = 2;

    path = sqlite3_mprintf ("./%s_sect1_triple_tif_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteSectionTripleBandTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    sqlite3_bind_int (stmt, 6, red_band);
    sqlite3_bind_int (stmt, 7, green_band);
    sqlite3_bind_int (stmt, 8, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 9, blob, blob_size, free);
    sqlite3_bind_double (stmt, 10, xx_res);
    sqlite3_bind_double (stmt, 11, yy_res);
    sqlite3_bind_text (stmt, 12, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_triple_tiff_tfw (sqlite3 * sqlite, const char *coverage,
				   gaiaGeomCollPtr geom, int scale)
{
/* exporting a Section TripleBand TiffTfw */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 0;
    unsigned char green_band = 1;
    unsigned char blue_band = 2;

    path = sqlite3_mprintf ("./%s_sect1_triple_tfw_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteSectionTripleBandTiffTfw('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    sqlite3_bind_int (stmt, 6, red_band);
    sqlite3_bind_int (stmt, 7, green_band);
    sqlite3_bind_int (stmt, 8, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 9, blob, blob_size, free);
    sqlite3_bind_double (stmt, 10, xx_res);
    sqlite3_bind_double (stmt, 11, yy_res);
    sqlite3_bind_text (stmt, 12, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path = sqlite3_mprintf ("./%s_sect1_triple_tfw_%d.tfw", coverage, scale);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_mono_geotiff (sqlite3 * sqlite, const char *coverage,
				gaiaGeomCollPtr geom, int scale)
{
/* exporting a Section MonoBand GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;

    path = sqlite3_mprintf ("./%s_sect1_mono_gt_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteSectionMonoBandGeoTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    sqlite3_bind_int (stmt, 6, 0);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 7, blob, blob_size, free);
    sqlite3_bind_double (stmt, 8, xx_res);
    sqlite3_bind_double (stmt, 9, yy_res);
    sqlite3_bind_int (stmt, 10, 0);
    sqlite3_bind_text (stmt, 11, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_mono_tiff (sqlite3 * sqlite, const char *coverage,
			     gaiaGeomCollPtr geom, int scale)
{
/* exporting a Section MonoBand Tiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;

    path = sqlite3_mprintf ("./%s_sect1_mono_tif_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteSectionMonoBandTiff('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    sqlite3_bind_int (stmt, 6, 0);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 7, blob, blob_size, free);
    sqlite3_bind_double (stmt, 8, xx_res);
    sqlite3_bind_double (stmt, 9, yy_res);
    sqlite3_bind_text (stmt, 10, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_mono_tiff_tfw (sqlite3 * sqlite, const char *coverage,
				 gaiaGeomCollPtr geom, int scale)
{
/* exporting a Section MonoBand Tiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;

    path = sqlite3_mprintf ("./%s_sect1_mono_tfw_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteSectionMonoBandTiffTfw('main', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    sqlite3_bind_int (stmt, 6, 0);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 7, blob, blob_size, free);
    sqlite3_bind_double (stmt, 8, xx_res);
    sqlite3_bind_double (stmt, 9, yy_res);
    sqlite3_bind_text (stmt, 10, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path = sqlite3_mprintf ("./%s_sect1_mono_tfw_%d.tfw", coverage, scale);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_map_image (sqlite3 * sqlite, const char *coverage,
		     gaiaGeomCollPtr geom, const char *style,
		     const char *suffix, int monolithic)
{
/* exporting a Map Image (full rendered) */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    const char *format = "text/plain";

    if (strcmp (suffix, "png") == 0)
	format = "image/png";
    if (strcmp (suffix, "jpg") == 0)
	format = "image/jpeg";
    if (strcmp (suffix, "tif") == 0)
	format = "image/tiff";
    if (strcmp (suffix, "pdf") == 0)
	format = "application/x-pdf";

    path =
	sqlite3_mprintf ("./%s_%s_map_%s.%s", coverage,
			 monolithic ? "mono" : "sect", style, suffix);

    sql =
	"SELECT BlobToFile(RL2_GetMapImageFromRaster('main', ?, ST_Buffer(?, 100), ?, ?, ?, ?, ?, ?), ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_text (stmt, 5, style, strlen (style), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 6, format, strlen (format), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 7, "#ffe0e0", 7, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 8, 1);
    sqlite3_bind_text (stmt, 9, path, strlen (path), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_ndvi (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom)
{
/* exporting an NDVI Ascii Grid */
    char *sql;
    const char *path = "./ndvi_ascii_grid.asc";
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    double scale = 1.0;

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;
    if (xx_res != yy_res)
	xx_res = (xx_res + yy_res) / 2.0;

    sql = "SELECT RL2_WriteNdviAsciiGrid('main', ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 0);	/* red band */
    sqlite3_bind_int (stmt, 6, 3);	/* NIR band */
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 7, blob, blob_size, free);
    sqlite3_bind_double (stmt, 8, xx_res);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    return retcode;
}

static int
do_export_section_ndvi (sqlite3 * sqlite, const char *coverage,
			gaiaGeomCollPtr geom)
{
/* exporting a Section NDVI Ascii Grid */
    char *sql;
    const char *path = "./ndvi_ascii_grid_section.asc";
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    double scale = 1.0;

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;
    if (xx_res != yy_res)
	xx_res = (xx_res + yy_res) / 2.0;

    sql =
	"SELECT RL2_WriteSectionNdviAsciiGrid('main', ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    sqlite3_bind_int (stmt, 6, 0);	/* red band */
    sqlite3_bind_int (stmt, 7, 3);	/* NIR band */
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, free);
    sqlite3_bind_double (stmt, 9, xx_res);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    return retcode;
}

static gaiaGeomCollPtr
get_center_point (sqlite3 * sqlite, const char *coverage)
{
/* attempting to retrieve the Coverage's Center Point */
    char *sql;
    sqlite3_stmt *stmt;
    gaiaGeomCollPtr geom = NULL;
    int ret;

    sql = sqlite3_mprintf ("SELECT MakePoint("
			   "extent_minx + ((extent_maxx - extent_minx) / 2.0), "
			   "extent_miny + ((extent_maxy - extent_miny) / 2.0)) "
			   "FROM raster_coverages WHERE coverage_name = %Q",
			   coverage);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return NULL;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		int blob_sz = sqlite3_column_bytes (stmt, 0);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
	    }
      }
    sqlite3_finalize (stmt);
    return geom;
}

static int
test_default_bands (sqlite3 * sqlite, const char *coverage)
{
/* testing default bands SQL functions */
    int ret;
    char *sql;
    char *err_msg = NULL;

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 4, 1, 2, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #1 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 4, 2, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #2 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 4, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #3 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 4, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #4 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 2, 4)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #5 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 0, 2, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #6 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 0, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #7 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 2, 0)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #8 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 1, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #9 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 2, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #10 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 2, 2)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #11 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf ("SELECT RL2_EnableRasterCoverageAutoNDVI(%Q, 1)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "EnableRasterCoverageAutoNDVI #1 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_IsRasterCoverageAutoNdviEnabled('main', %Q)", coverage);
    ret = execute_check_boolean (sqlite, sql);
    sqlite3_free (sql);
    if (ret >= 0)
      {
	  fprintf (stderr,
		   "IsRasterCoverageAutoNdviEnabled #1 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_SetRasterCoverageDefaultBands(%Q, 0, 1, 2, 3)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "SetRasterCoverageDefaultBands #12 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf ("SELECT RL2_EnableRasterCoverageAutoNDVI(%Q, 0)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "EnableRasterCoverageAutoNDVI #2 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_IsRasterCoverageAutoNdviEnabled('main', %Q)", coverage);
    ret = execute_check_boolean (sqlite, sql);
    sqlite3_free (sql);
    if (ret != 0)
      {
	  fprintf (stderr,
		   "IsRasterCoverageAutoNdviEnabled #2 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf ("SELECT RL2_EnableRasterCoverageAutoNDVI(%Q, 1)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "EnableRasterCoverageAutoNDVI #3 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	sqlite3_mprintf
	("SELECT RL2_IsRasterCoverageAutoNdviEnabled('main', %Q)", coverage);
    ret = execute_check_boolean (sqlite, sql);
    sqlite3_free (sql);
    if (ret != 1)
      {
	  fprintf (stderr,
		   "IsRasterCoverageAutoNdviEnabled #3 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_test_tile_data (sqlite3 * sqlite, unsigned char compression, int tile_sz)
{
/* testing some DBMS Coverage's Tile Data */
    int ret;
    const char *coverage = NULL;
    char *sql;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt;
    int err = 0;

/* setting the coverage name */
    switch (compression)
      {
      case RL2_COMPRESSION_CHARLS:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_charls_256";
		break;
	    case TILE_512:
		coverage = "infrared_charls_512";
		break;
	    case TILE_1024:
		coverage = "infrared_charls_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_LOSSY_JP2:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_jp2_256";
		break;
	    case TILE_512:
		coverage = "infrared_jp2_512";
		break;
	    case TILE_1024:
		coverage = "infrared_jp2_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_PNG:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_png_256";
		break;
	    case TILE_512:
		coverage = "infrared_png_512";
		break;
	    case TILE_1024:
		coverage = "infrared_png_1024";
		break;
	    };
	  break;
      };

    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even FROM \"%s\"",
			 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		unsigned int width;
		unsigned int height;
		unsigned char sample_type;
		unsigned char pixel_type;
		unsigned char num_bands;
		unsigned char compr;
		int is_odd_tile;
		int has_mask;
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      if (rl2_query_dbms_raster_tile
			  (blob, blob_sz, &width, &height, &sample_type,
			   &pixel_type, &num_bands, &compr, &is_odd_tile,
			   &has_mask) == RL2_OK)
			{
			    int err_x = 0;
			    if (is_odd_tile != 1)
				err_x = 1;
			    if (width != (unsigned int) tile_sz)
				err_x = 1;
			    if (height != (unsigned int) tile_sz)
				err = 1;
			    if (sample_type != 0xa5)
				err = 1;
			    if (pixel_type != 0x15)
				err = 1;
			    if (num_bands != 4)
				err = 1;
			    if (compr != compression)
				err_x = 1;
			    if (err_x)
			      {
				  fprintf (stderr, "INVALID ODD TILE\n");
				  err = 1;
			      }
			}
		      else
			{
			    fprintf (stderr, "INVALID ODD TILE\n");
			    err = 1;
			}
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 1);
		      int blob_sz = sqlite3_column_bytes (stmt, 1);
		      if (rl2_query_dbms_raster_tile
			  (blob, blob_sz, &width, &height, &sample_type,
			   &pixel_type, &num_bands, &compr, &is_odd_tile,
			   &has_mask) == RL2_OK)
			{
			    int err_x = 0;
			    if (is_odd_tile != 0)
				err_x = 1;
			    if (width != (unsigned int) tile_sz)
				err_x = 1;
			    if (height != (unsigned int) tile_sz)
				err = 1;
			    if (sample_type != 0xa5)
				err = 1;
			    if (pixel_type != 0x15)
				err = 1;
			    if (num_bands != 4)
				err = 1;
			    if (compr != compression)
				err_x = 1;
			    if (err_x)
			      {
				  fprintf (stderr, "INVALID EVEN TILE\n");
				  err = 1;
			      }
			}
		      else
			{
			    fprintf (stderr, "INVALID EVEN TILE\n");
			    err = 1;
			}
		  }
	    }
      }
    sqlite3_finalize (stmt);
    if (err)
	return 0;
    return 1;
}

static int
test_pixel (sqlite3 * sqlite, int *retcode)
{
/* testing GetPixelFromRasterByPoint() */
    int ret;
    char *err_msg = NULL;
    const char *sql;
    int value;

/* testing band #0 */
    sql =
	"SELECT RL2_GetPixelValue(RL2_GetPixelFromRasterByPoint(NULL, 'infrared_png_1024', MakePoint(661136.0, 4847595.0, 32632), 0), 0)";
    ret = execute_check_value (sqlite, sql, &value);
    if (!ret)
      {
	  fprintf (stderr, "GetPixelFromRasterByPoint #0 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (value != 215)
      {
	  fprintf (stderr,
		   "GetPixelFromRasterByPoint #0 error: expected 215, found %d\n",
		   value);
	  *retcode += -1;
	  return 0;
      }

/* testing band #1 */
    sql =
	"SELECT RL2_GetPixelValue(RL2_GetPixelFromRasterByPoint(NULL, 'infrared_png_1024', MakePoint(661136.0, 4847595.0, 32632), 0), 1)";
    ret = execute_check_value (sqlite, sql, &value);
    if (!ret)
      {
	  fprintf (stderr, "GetPixelFromRasterByPoint #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (value != 201)
      {
	  fprintf (stderr,
		   "GetPixelFromRasterByPoint #2 error: expected 201, found %d\n",
		   value);
	  *retcode += -1;
	  return 0;
      }

/* testing band #2 */
    sql =
	"SELECT RL2_GetPixelValue(RL2_GetPixelFromRasterByPoint(NULL, 'infrared_png_1024', MakePoint(661136.0, 4847595.0, 32632), 0), 2)";
    ret = execute_check_value (sqlite, sql, &value);
    if (!ret)
      {
	  fprintf (stderr, "GetPixelFromRasterByPoint #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (value != 192)
      {
	  fprintf (stderr,
		   "GetPixelFromRasterByPoint #3 error: expected 192, found %d\n",
		   value);
	  *retcode += -1;
	  return 0;
      }

/* testing band #3 */
    sql =
	"SELECT RL2_GetPixelValue(RL2_GetPixelFromRasterByPoint(NULL, 'infrared_png_1024', MakePoint(661136.0, 4847595.0, 32632), 0), 3)";
    ret = execute_check_value (sqlite, sql, &value);
    if (!ret)
      {
	  fprintf (stderr, "GetPixelFromRasterByPoint #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (value != 172)
      {
	  fprintf (stderr,
		   "GetPixelFromRasterByPoint #4 error: expected 172, found %d\n",
		   value);
	  *retcode += -1;
	  return 0;
      }
    return 1;
}

static int
test_coverage (sqlite3 * sqlite, unsigned char compression, int tile_sz,
	       int ndvi, int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    const char *sample_name = NULL;
    const char *pixel_name = NULL;
    unsigned char num_bands = 1;
    const char *compression_name = NULL;
    int qlty = 100;
    int tile_size = 256;
    char *sql;
    gaiaGeomCollPtr geom;
    int test_map_image = 0;

/* setting the coverage name */
    switch (compression)
      {
      case RL2_COMPRESSION_CHARLS:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_charls_256";
		break;
	    case TILE_512:
		coverage = "infrared_charls_512";
		break;
	    case TILE_1024:
		coverage = "infrared_charls_1024";
		test_map_image = 1;
		break;
	    };
	  break;
      case RL2_COMPRESSION_LOSSY_JP2:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_jp2_256";
		break;
	    case TILE_512:
		coverage = "infrared_jp2_512";
		break;
	    case TILE_1024:
		coverage = "infrared_jp2_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_PNG:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_png_256";
		break;
	    case TILE_512:
		coverage = "infrared_png_512";
		break;
	    case TILE_1024:
		coverage = "infrared_png_1024";
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    sample_name = "UINT8";
    pixel_name = "MULTIBAND";
    num_bands = 4;
    switch (compression)
      {
      case RL2_COMPRESSION_CHARLS:
	  compression_name = "CHARLS";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_LOSSY_JP2:
	  compression_name = "JP2";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_PNG:
	  compression_name = "PNG";
	  qlty = 100;
	  break;
      };
    switch (tile_sz)
      {
      case TILE_256:
	  tile_size = 256;
	  break;
      case TILE_512:
	  tile_size = 512;
	  break;
      case TILE_1024:
	  tile_size = 1024;
	  break;
      };

/* creating the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.2f, %1.2f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 32632,
			   0.2, 0.2);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* loading the first section */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRaster(%Q, %Q, 0, 32632, 0, 1)", coverage,
	 "map_samples/tuscany-infrared/infrared1.tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRasters #1 \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -2;
	  return 0;
      }

/* loading the second section */
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 1, 32632, 0, 1)",
			   coverage,
			   "map_samples/tuscany-infrared/infrared2.tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster #2 \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -3;
	  return 0;
      }

/* re-building the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, 2, 1, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
	  return 0;
      }

/* destroying the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_DePyramidize(%Q, NULL, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DePyramidize \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -5;
	  return 0;
      }

/* building yet again the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, 2, 1, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -6;
	  return 0;
      }

/* export tests */
    geom = get_center_point (sqlite, coverage);
    if (geom == NULL)
      {
	  *retcode += -7;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 1))
      {
	  *retcode += -8;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 2))
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 4))
      {
	  *retcode += -10;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 8))
      {
	  *retcode += -11;
	  return 0;
      }

/* testing TripleBand (Geo)TIFF export */
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 1, 0, 0))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 1, 1, 0))
      {
	  *retcode += -13;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 1, 2, 1))
      {
	  *retcode += -14;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 4, 0, 0))
      {
	  *retcode += -15;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 4, 1, 1))
      {
	  *retcode += -16;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 4, 2, 0))
      {
	  *retcode += -17;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 8, 0, 1))
      {
	  *retcode += -18;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 8, 1, 0))
      {
	  *retcode += -19;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 8, 2, 0))
      {
	  *retcode += -20;
	  return 0;
      }

    if (!do_export_band_composed_tiff (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -21;
	  return 0;
      }
    if (!do_export_band_composed_tiff (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -22;
	  return 0;
      }
    if (!do_export_band_composed_tiff (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -23;
	  return 0;
      }

    if (!do_export_band_composed_tiff_tfw (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -24;
	  return 0;
      }
    if (!do_export_band_composed_tiff_tfw (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -25;
	  return 0;
      }
    if (!do_export_band_composed_tiff_tfw (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -26;
	  return 0;
      }

/* testing MonoBand (Geo)TIFF export */
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 1, 0, 0))
      {
	  *retcode += -27;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 1, 1, 0))
      {
	  *retcode += -28;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 1, 2, 1))
      {
	  *retcode += -29;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 4, 0, 0))
      {
	  *retcode += -30;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 4, 1, 1))
      {
	  *retcode += -31;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 4, 2, 0))
      {
	  *retcode += -32;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 8, 0, 1))
      {
	  *retcode += -33;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 8, 1, 0))
      {
	  *retcode += -34;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 8, 2, 0))
      {
	  *retcode += -35;
	  return 0;
      }

    if (!do_export_mono_band_tiff (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -36;
	  return 0;
      }
    if (!do_export_mono_band_tiff (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -37;
	  return 0;
      }
    if (!do_export_mono_band_tiff (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -38;
	  return 0;
      }

    if (!do_export_mono_band_tiff_tfw (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -39;
	  return 0;
      }
    if (!do_export_mono_band_tiff_tfw (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -40;
	  return 0;
      }
    if (!do_export_mono_band_tiff_tfw (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -41;
	  return 0;
      }
    if (ndvi)
      {
	  /* NDVI Ascii Grid */
	  if (!do_export_ndvi (sqlite, coverage, geom))
	    {
		*retcode += -101;
		return 0;
	    }
      }
    if (!test_map_image)
	goto skip;

/* loading the RasterSymbolizers */
    sql =
	sqlite3_mprintf ("SELECT SE_RegisterRasterStyledLayer(%Q, 1)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #1 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -42;
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT SE_RegisterRasterStyledLayer(%Q, 2)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #2 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -43;
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT SE_RegisterRasterStyledLayer(%Q, 3)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #3 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -44;
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT SE_RegisterRasterStyledLayer(%Q, 4)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #4 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -45;
	  return 0;
      }
    sql =
	sqlite3_mprintf ("SELECT SE_RegisterRasterStyledLayer(%Q, 5)",
			 coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #5 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -103;
	  return 0;
      }

/* testing GetMapImage - NDVI */
    if (!test_default_bands (sqlite, coverage))
      {
	  *retcode += -102;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ndvi", "jpg", 0))
      {
	  *retcode += 104;
	  return 0;
      }

/* testing GetMapImage - IR false color */
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1", "png", 0))
      {
	  *retcode += 46;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1", "jpg", 0))
      {
	  *retcode += 47;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1", "tif", 0))
      {
	  *retcode += 48;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1", "pdf", 0))
      {
	  *retcode += 49;
	  return 0;
      }

/* testing GetMapImage - IR false color - GammaValue */
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1_gamma", "png", 0))
      {
	  *retcode += 50;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1_gamma", "jpg", 0))
      {
	  *retcode += 51;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1_gamma", "tif", 0))
      {
	  *retcode += 52;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color1_gamma", "pdf", 0))
      {
	  *retcode += 53;
	  return 0;
      }

/* testing GetMapImage - IR gray */
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "png", 0))
      {
	  *retcode += 54;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "jpg", 0))
      {
	  *retcode += 55;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "tif", 0))
      {
	  *retcode += 56;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "pdf", 0))
      {
	  *retcode += 57;
	  return 0;
      }

/* testing GetMapImage - IR gray - GammaValue */
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_gray_gamma", "png", 0))
      {
	  *retcode += 58;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_gray_gamma", "jpg", 0))
      {
	  *retcode += 59;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_gray_gamma", "tif", 0))
      {
	  *retcode += 60;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_gray_gamma", "pdf", 0))
      {
	  *retcode += 61;
	  return 0;
      }
  skip:
    gaiaFreeGeomColl (geom);

/* testing GetTileImage() - basic */
    if (!do_export_tile_image (sqlite, coverage, 1))
      {
	  *retcode += -162;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, -1))
      {
	  *retcode += -163;
	  return 0;
      }

/* testing GetTileImage() - triple band */
    if (!do_export_tile_image3 (sqlite, coverage, 1, 0))
      {
	  *retcode += -62;
	  return 0;
      }
    if (!do_export_tile_image3 (sqlite, coverage, 1, 1))
      {
	  *retcode += -63;
	  return 0;
      }
    if (!do_export_tile_image3 (sqlite, coverage, 1, 2))
      {
	  *retcode += -64;
	  return 0;
      }
    if (!do_export_tile_image3 (sqlite, coverage, -1, 0))
      {
	  *retcode += -65;
	  return 0;
      }

/* testing GetTileImage() - Mono-Band */
    if (!do_export_mono_tile_image (sqlite, coverage, 1, 0))
      {
	  *retcode += -66;
	  return 0;
      }
    if (!do_export_mono_tile_image (sqlite, coverage, 1, 1))
      {
	  *retcode += -67;
	  return 0;
      }
    if (!do_export_mono_tile_image (sqlite, coverage, 1, 2))
      {
	  *retcode += -68;
	  return 0;
      }
    if (!do_export_mono_tile_image (sqlite, coverage, -1, 0))
      {
	  *retcode += -69;
	  return 0;
      }

    if (compression == RL2_COMPRESSION_CHARLS && tile_sz == TILE_1024)
      {
	  /* testing a Monolithic Pyramid */
	  geom = get_center_point (sqlite, coverage);
	  if (geom == NULL)
	    {
		*retcode += -70;
		return 0;
	    }
	  sql =
	      sqlite3_mprintf ("SELECT RL2_PyramidizeMonolithic(%Q, 1, 1)",
			       coverage);
	  ret = execute_check (sqlite, sql);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "PyramidizeMonolithic \"%s\" error: %s\n",
			 coverage, err_msg);
		sqlite3_free (err_msg);
		*retcode += -71;
		return 0;
	    }
	  if (!do_export_map_image
	      (sqlite, coverage, geom, "ir_false_color1", "jpg", 1))
	    {
		*retcode += 71;
		return 0;
	    }
	  if (!do_export_map_image
	      (sqlite, coverage, geom, "ir_false_color1_gamma", "jpg", 1))
	    {
		*retcode += 72;
		return 0;
	    }
	  if (!do_export_map_image
	      (sqlite, coverage, geom, "ir_gray", "jpg", 1))
	    {
		*retcode += 73;
		return 0;
	    }
	  if (!do_export_map_image
	      (sqlite, coverage, geom, "ir_gray_gamma", "jpg", 1))
	    {
		*retcode += 74;
		return 0;
	    }
	  gaiaFreeGeomColl (geom);
      }

    if (strcmp (coverage, "infrared_png_512") == 0)
      {
	  /* testing Band Composed Section */
	  geom = get_center_point (sqlite, coverage);
	  if (geom == NULL)
	    {
		*retcode += -75;
		return 0;
	    }
	  /* GeoTiff TripleBand */
	  if (!do_export_section_triple_geotiff (sqlite, coverage, geom, 1))
	    {
		*retcode += -76;
		return 0;
	    }
	  if (!do_export_section_triple_geotiff (sqlite, coverage, geom, 2))
	    {
		*retcode += -77;
		return 0;
	    }
	  if (!do_export_section_triple_geotiff (sqlite, coverage, geom, 4))
	    {
		*retcode += -78;
		return 0;
	    }
	  if (!do_export_section_triple_geotiff (sqlite, coverage, geom, 8))
	    {
		*retcode += -79;
		return 0;
	    }
	  /* Tiff TripleBand */
	  if (!do_export_section_triple_tiff (sqlite, coverage, geom, 1))
	    {
		*retcode += -80;
		return 0;
	    }
	  if (!do_export_section_triple_tiff (sqlite, coverage, geom, 2))
	    {
		*retcode += -81;
		return 0;
	    }
	  if (!do_export_section_triple_tiff (sqlite, coverage, geom, 4))
	    {
		*retcode += -82;
		return 0;
	    }
	  if (!do_export_section_triple_tiff (sqlite, coverage, geom, 8))
	    {
		*retcode += -83;
		return 0;
	    }
	  /* TiffTfw TripleBand */
	  if (!do_export_section_triple_tiff_tfw (sqlite, coverage, geom, 1))
	    {
		*retcode += -84;
		return 0;
	    }
	  if (!do_export_section_triple_tiff_tfw (sqlite, coverage, geom, 2))
	    {
		*retcode += -85;
		return 0;
	    }
	  if (!do_export_section_triple_tiff_tfw (sqlite, coverage, geom, 4))
	    {
		*retcode += -86;
		return 0;
	    }
	  if (!do_export_section_triple_tiff_tfw (sqlite, coverage, geom, 8))
	    {
		*retcode += -87;
		return 0;
	    }
	  /* GeoTiff MonoBand */
	  if (!do_export_section_mono_geotiff (sqlite, coverage, geom, 1))
	    {
		*retcode += -88;
		return 0;
	    }
	  if (!do_export_section_mono_geotiff (sqlite, coverage, geom, 2))
	    {
		*retcode += -89;
		return 0;
	    }
	  if (!do_export_section_mono_geotiff (sqlite, coverage, geom, 4))
	    {
		*retcode += -90;
		return 0;
	    }
	  if (!do_export_section_mono_geotiff (sqlite, coverage, geom, 8))
	    {
		*retcode += -91;
		return 0;
	    }
	  /* Tiff MonoBand */
	  if (!do_export_section_mono_tiff (sqlite, coverage, geom, 1))
	    {
		*retcode += -92;
		return 0;
	    }
	  if (!do_export_section_mono_tiff (sqlite, coverage, geom, 2))
	    {
		*retcode += -93;
		return 0;
	    }
	  if (!do_export_section_mono_tiff (sqlite, coverage, geom, 4))
	    {
		*retcode += -94;
		return 0;
	    }
	  if (!do_export_section_mono_tiff (sqlite, coverage, geom, 8))
	    {
		*retcode += -95;
		return 0;
	    }
	  /* TiffTfw MonoBand */
	  if (!do_export_section_mono_tiff_tfw (sqlite, coverage, geom, 1))
	    {
		*retcode += -97;
		return 0;
	    }
	  if (!do_export_section_mono_tiff_tfw (sqlite, coverage, geom, 2))
	    {
		*retcode += -98;
		return 0;
	    }
	  if (!do_export_section_mono_tiff_tfw (sqlite, coverage, geom, 4))
	    {
		*retcode += -99;
		return 0;
	    }
	  if (!do_export_section_mono_tiff_tfw (sqlite, coverage, geom, 8))
	    {
		*retcode += -100;
		return 0;
	    }
	  if (ndvi)
	    {
		/* Section NDVI Ascii Grid */
		if (!do_export_section_ndvi (sqlite, coverage, geom))
		  {
		      *retcode += -102;
		      return 0;
		  }
	    }
	  gaiaFreeGeomColl (geom);
      }

    if (!do_test_tile_data (sqlite, compression, tile_sz))
      {
	  *retcode += -103;
	  return 0;
      }

    return 1;
}

static int
register_raster_symbolizers (sqlite3 * sqlite, int no_web_connection,
			     int *retcode)
{
/* loading the RasterSymbolizers */
    int ret;
    char *err_msg = NULL;
    char *sql;

    if (no_web_connection)
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1))",
	     "ir_false_color1.xml");
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1, 1))",
	     "ir_false_color1.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyle #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (no_web_connection)
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1))",
	     "ir_false_color1_gamma.xml");
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1, 1))",
	     "ir_false_color1_gamma.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyle #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -2;
	  return 0;
      }
    if (no_web_connection)
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1))",
	     "ir_gray.xml");
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1, 1))",
	     "ir_gray.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyle #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -3;
	  return 0;
      }
    if (no_web_connection)
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1))",
	     "ir_gray_gamma.xml");
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1, 1))",
	     "ir_gray_gamma.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
	  return 0;
      }
    if (no_web_connection)
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1))",
	     "ndvi.xml");
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(%Q), 1, 1))",
	     "ndvi.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyle #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -5;
	  return 0;
      }

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char compression, int tile_sz,
	       int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    char *sql;

/* setting the coverage name */
    switch (compression)
      {
      case RL2_COMPRESSION_CHARLS:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_charls_256";
		break;
	    case TILE_512:
		coverage = "infrared_charls_512";
		break;
	    case TILE_1024:
		coverage = "infrared_charls_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_LOSSY_JP2:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_jp2_256";
		break;
	    case TILE_512:
		coverage = "infrared_jp2_512";
		break;
	    case TILE_1024:
		coverage = "infrared_jp2_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_PNG:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "infrared_png_256";
		break;
	    case TILE_512:
		coverage = "infrared_png_512";
		break;
	    case TILE_1024:
		coverage = "infrared_png_1024";
		break;
	    };
	  break;
      };

/* dropping the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_DropRasterCoverage(%Q, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropRasterCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

    return 1;
}

int
main (int argc, char *argv[])
{
    int no_web_connection = 0;
    int result = 0;
    int ret;
    char *err_msg = NULL;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (getenv ("ENABLE_RL2_WEB_TESTS") == NULL)
      {
	  fprintf (stderr,
		   "this testcase has been executed with several limitations\n"
		   "because it was not enabled to access the Web.\n\n"
		   "you can enable all testcases requiring an Internet connection\n"
		   "by setting the environment variable \"ENABLE_RL2_WEB_TESTS=1\"\n\n");
	  no_web_connection = 1;
      }

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);
    ret =
	sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT CreateRasterCoveragesTable()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -3;
      }
    if (no_web_connection)
	ret =
	    sqlite3_exec (db_handle, "SELECT CreateStylingTables(1)", NULL,
			  NULL, &err_msg);
    else
	ret =
	    sqlite3_exec (db_handle, "SELECT CreateStylingTables()", NULL,
			  NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateStylingTables() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -4;
      }
    if (!register_raster_symbolizers (db_handle, no_web_connection, &ret))
      {
	  fprintf (stderr, "Register Raster Symbolizers error\n");
	  return -5;
      }

/* tests */
#ifndef OMIT_CHARLS		/* only if CharLS is enabled */
    ret = -100;
    if (!test_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_256, 0, &ret))
	return ret;
    ret = -120;
    if (!test_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_512, 0, &ret))
	return ret;
    ret = -140;
    if (!test_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_1024, 0, &ret))
	return ret;
#endif /* end CharLS conditional */

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
    ret = -200;
    if (!test_coverage
	(db_handle, RL2_COMPRESSION_LOSSY_JP2, TILE_256, 0, &ret))
	return ret;
    ret = -220;
    if (!test_coverage
	(db_handle, RL2_COMPRESSION_LOSSY_JP2, TILE_512, 0, &ret))
	return ret;
    ret = -240;
    if (!test_coverage
	(db_handle, RL2_COMPRESSION_LOSSY_JP2, TILE_1024, 0, &ret))
	return ret;
#endif /* end OpenJpeg conditional */

    ret = -300;
    if (!test_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_256, 0, &ret))
	return ret;
    ret = -320;
    if (!test_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_512, 1, &ret))
	return ret;
    ret = -340;
    if (!test_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_1024, 0, &ret))
	return ret;
    ret = -350;
    if (!test_pixel (db_handle, &ret))
	return ret;

/* dropping all Coverages */
#ifndef OMIT_CHARLS		/* only if CharLS is enabled */
    ret = -170;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_512, &ret))
	return ret;
    ret = -190;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_1024, &ret))
	return ret;
#endif /* end CharLS conditional */

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
    ret = -270;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_LOSSY_JP2, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_LOSSY_JP2, TILE_512, &ret))
	return ret;
    ret = -290;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_LOSSY_JP2, TILE_1024, &ret))
	return ret;
#endif /* end OpenJpeg conditional */

    ret = -370;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -380;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -390;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    if (old_SPATIALITE_SECURITY_ENV)
      {
#ifdef _WIN32
	  char *env = sqlite3_mprintf ("SPATIALITE_SECURITY=%s",
				       old_SPATIALITE_SECURITY_ENV);
	  putenv (env);
	  sqlite3_free (env);
#else /* not WIN32 */
	  setenv ("SPATIALITE_SECURITY", old_SPATIALITE_SECURITY_ENV, 1);
#endif
      }
    else
      {
#ifdef _WIN32
	  putenv ("SPATIALITE_SECURITY=");
#else /* not WIN32 */
	  unsetenv ("SPATIALITE_SECURITY");
#endif
      }
    return result;
}
