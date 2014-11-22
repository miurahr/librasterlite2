/*

 test_map_mono.c -- RasterLite-2 Test Case

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

The Original Code is the SpatiaLite library

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
/* attempting to export a visible Tile */
    char *sql;
    char *path;
    int ret;
    int transparent = 1;

    if (tile_id <= 1)
	transparent = 0;

    if (tile_id < 0)
	tile_id = get_max_tile_id (sqlite, coverage);
    path = sqlite3_mprintf ("./%s_tile_%d.png", coverage, tile_id);
    sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetTileImage(%Q, %d, '#e0ffe0', %d), %Q)",
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
do_export_geotiff (sqlite3 * sqlite, const char *coverage, const char *type,
		   gaiaGeomCollPtr geom, int scale, const char *compression)
{
/* exporting a GeoTiff + Worldfile */
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

    path = sqlite3_mprintf ("./%s_%s_gt_%d.tif", coverage, type, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteGeoTiff(?, ?, ?, ?, ?, ?, ?, ?, ?)";
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
    sqlite3_bind_int (stmt, 8, 1);
    sqlite3_bind_text (stmt, 9, compression, strlen (compression),
		       SQLITE_TRANSIENT);
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
    path = sqlite3_mprintf ("./%s_%s_gt_%d.tfw", coverage, type, scale);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_tiff (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		int scale, const char *compression)
{
/* exporting a Tiff (no Worldfile) */
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

    path = sqlite3_mprintf ("./%s_plain_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteTiff(?, ?, ?, ?, ?, ?, ?, ?)";
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
    sqlite3_bind_text (stmt, 8, compression, strlen (compression),
		       SQLITE_TRANSIENT);
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
do_export_tiff_tfw (sqlite3 * sqlite, const char *coverage,
		    gaiaGeomCollPtr geom, int scale, const char *compression)
{
/* exporting a Tiff (+ Worldfile) */
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

    path = sqlite3_mprintf ("./%s_plain_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteTiffTfw(?, ?, ?, ?, ?, ?, ?, ?)";
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
    sqlite3_bind_text (stmt, 8, compression, strlen (compression),
		       SQLITE_TRANSIENT);
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
    path = sqlite3_mprintf ("./%s_plain_%d.tfw", coverage, scale);
    unlink (path);
    sqlite3_free (path);
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
do_export_image (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		 double radius, const char *suffix, int transparent)
{
/* exporting a PNG/JPEG/TIFF/PDF image */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    const char *mime_type = "text/plain";

    path = sqlite3_mprintf ("./%s_%1.0f%s", coverage, radius, suffix);

    sql =
	"SELECT RL2_GetMapImage(?, ST_Buffer(?, ?), 512, 512, 'default', ?, '#ffffff', ?, 80)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
    sqlite3_bind_double (stmt, 3, radius);
    if (strcmp (suffix, ".png") == 0)
	mime_type = "image/png";
    if (strcmp (suffix, ".jpg") == 0)
	mime_type = "image/jpeg";
    if (strcmp (suffix, ".tif") == 0)
	mime_type = "image/tiff";
    if (strcmp (suffix, ".pdf") == 0)
	mime_type = "application/x-pdf";
    sqlite3_bind_text (stmt, 4, mime_type, strlen (mime_type),
		       SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 5, transparent);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
	    {
		FILE *out;
		blob = (unsigned char *) sqlite3_column_blob (stmt, 0);
		blob_size = sqlite3_column_bytes (stmt, 0);
		out = fopen (path, "wb");
		if (out != NULL)
		  {
		      /* writing the output image */
		      if ((int) fwrite (blob, 1, blob_size, out) == blob_size)
			  retcode = 1;
		      fclose (out);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to GetMap \"%s\"\n", path);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
test_coverage (sqlite3 * sqlite, unsigned char pixel,
	       unsigned char compression, int tile_sz, int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    const char *sample_name = NULL;
    const char *pixel_name = NULL;
    unsigned char num_bands = 1;
    const char *compression_name = NULL;
    const char *alt_compression_name = "NONE";
    int qlty = 100;
    char *sql;
    int tile_size = 256;
    gaiaGeomCollPtr geom;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_none_256";
		      break;
		  case TILE_512:
		      coverage = "mono_none_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_png_256";
		      break;
		  case TILE_512:
		      coverage = "mono_png_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_CCITTFAX4:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_fax4_256";
		      break;
		  case TILE_512:
		      coverage = "mono_fax4_256_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_fax4_256_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_deflate_256";
		      break;
		  case TILE_512:
		      coverage = "mono_deflate_256_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_deflate_256_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "mono_lzma_256_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_lzma_256_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_none_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_none_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_png_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_png_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_deflate_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_deflate_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_lzma_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    sample_name = "1-BIT";
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  pixel_name = "MONOCHROME";
	  break;
      case RL2_PIXEL_PALETTE:
	  pixel_name = "PALETTE";
	  break;
      };
    num_bands = 1;
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  compression_name = "NONE";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  compression_name = "DEFLATE";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_DEFLATE_NO:
	  compression_name = "DEFLATE_NO";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_LZMA:
	  compression_name = "LZMA";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_LZMA_NO:
	  compression_name = "LZMA_NO";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_PNG:
	  compression_name = "PNG";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_CCITTFAX4:
	  compression_name = "FAX4";
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
    sql = sqlite3_mprintf ("SELECT RL2_CreateCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.2f, %1.2f, "
			   "RL2_SetPixelValue(RL2_CreatePixel(%Q, %Q, 1), 0, 0))",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 3003,
			   0.5, 0.5, sample_name, pixel_name);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* loading from directory */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 3003, 0, 1)", coverage,
	 "map_samples/ctrt", ".tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRastersFromDir \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -2;
	  return 0;
      }

/* deleting the first section */
    sql = sqlite3_mprintf ("SELECT RL2_DeleteSection(%Q, 1, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DeleteSection \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -3;
	  return 0;
      }

/* re-loading yet again the first section */
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, 3003, 1, 1)",
			   coverage, "map_samples/ctrt/cap1.tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
	  return 0;
      }

/* building the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, NULL, 0, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -5;
	  return 0;
      }

/* export tests */
    if (pixel == RL2_PIXEL_MONOCHROME)
	compression_name = "FAX3";
    else
	compression_name = "NONE";
    geom = get_center_point (sqlite, coverage);
    if (geom == NULL)
      {
	  *retcode += -6;
	  return 0;
      }
    if (!do_export_geotiff
	(sqlite, coverage, "norm", geom, 1, compression_name))
      {
	  *retcode += -7;
	  return 0;
      }

    if (!do_export_geotiff
	(sqlite, coverage, "norm", geom, 2, alt_compression_name))
      {
	  *retcode += -8;
	  return 0;
      }
    if (!do_export_geotiff
	(sqlite, coverage, "norm", geom, 4, alt_compression_name))
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_geotiff
	(sqlite, coverage, "norm", geom, 8, alt_compression_name))
      {
	  *retcode += -10;
	  return 0;
      }

    if (!do_export_tiff (sqlite, coverage, geom, 1, compression_name))
      {
	  *retcode += -11;
	  return 0;
      }

    if (!do_export_tiff (sqlite, coverage, geom, 2, alt_compression_name))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_tiff (sqlite, coverage, geom, 4, alt_compression_name))
      {
	  *retcode += -13;
	  return 0;
      }
    if (!do_export_tiff (sqlite, coverage, geom, 8, alt_compression_name))
      {
	  *retcode += -14;
	  return 0;
      }

    if (!do_export_tiff_tfw (sqlite, coverage, geom, 2, alt_compression_name))
      {
	  *retcode += -15;
	  return 0;
      }
    if (!do_export_tiff_tfw (sqlite, coverage, geom, 4, alt_compression_name))
      {
	  *retcode += -16;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 128.0, ".jpg", 1))
      {
	  *retcode += -17;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 290.0, ".jpg", 0))
      {
	  *retcode += -18;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 128.0, ".png", 1))
      {
	  *retcode += -19;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 60.0, ".png", 1))
      {
	  *retcode += -20;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 66.0, ".png", 0))
      {
	  *retcode += -21;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 128.0, ".tif", 1))
      {
	  *retcode += -23;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 290.0, ".tif", 0))
      {
	  *retcode += -24;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 128.0, ".pdf", 1))
      {
	  *retcode += -25;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 290.0, ".pdf", 0))
      {
	  *retcode += -26;
	  return 0;
      }

    gaiaFreeGeomColl (geom);

/* testing GetTileImage() */
    if (!do_export_tile_image (sqlite, coverage, 1))
      {
	  *retcode += -27;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, 27))
      {
	  *retcode += -28;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, -1))
      {
	  *retcode += -29;
	  return 0;
      }

    if (strcmp (coverage, "mono_fax4_256") == 0
	|| strcmp (coverage, "plt1_png_512") == 0)
      {
	  /* testing a Monolithic Pyramid */
	  geom = get_center_point (sqlite, coverage);
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
		*retcode += -30;
		return 0;
	    }

	  /* export tests */
	  if (geom == NULL)
	    {
		*retcode += -31;
		return 0;
	    }
	  if (!do_export_geotiff
	      (sqlite, coverage, "mono", geom, 1, compression_name))
	    {
		*retcode += -32;
		return 0;
	    }
	  if (!do_export_geotiff
	      (sqlite, coverage, "mono", geom, 2, alt_compression_name))
	    {
		*retcode += -33;
		return 0;
	    }
	  if (!do_export_geotiff
	      (sqlite, coverage, "mono", geom, 4, alt_compression_name))
	    {
		*retcode += -34;
		return 0;
	    }
	  if (!do_export_geotiff
	      (sqlite, coverage, "mono", geom, 8, alt_compression_name))
	    {
		*retcode += -35;
		return 0;
	    }
	  gaiaFreeGeomColl (geom);
      }

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char pixel,
	       unsigned char compression, int tile_sz, int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    char *sql;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_none_256";
		      break;
		  case TILE_512:
		      coverage = "mono_none_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_png_256";
		      break;
		  case TILE_512:
		      coverage = "mono_png_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_CCITTFAX4:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_fax4_256";
		      break;
		  case TILE_512:
		      coverage = "mono_fax4_256_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_fax4_256_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_deflate_256";
		      break;
		  case TILE_512:
		      coverage = "mono_deflate_256_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_deflate_256_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "mono_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "mono_lzma_256_512";
		      break;
		  case TILE_1024:
		      coverage = "mono_lzma_256_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_none_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_none_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_png_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_png_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_deflate_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_deflate_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt1_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "plt1_lzma_512";
		      break;
		  case TILE_1024:
		      coverage = "plt1_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      };

/* dropping the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_DropCoverage(%Q, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropCoverage \"%s\" error: %s\n", coverage,
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
    int result = 0;
    int ret;
    char *err_msg = NULL;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

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
    rl2_init (db_handle, 0);
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

/* MONOCHROME tests */
    ret = -100;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -120;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -140;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE, TILE_1024,
	 &ret))
	return ret;
    ret = -200;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -220;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -240;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -300;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_CCITTFAX4, TILE_256,
	 &ret))
	return ret;
    ret = -320;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_CCITTFAX4, TILE_512,
	 &ret))
	return ret;
    ret = -340;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_CCITTFAX4, TILE_1024,
	 &ret))
	return ret;
    ret = -360;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_DEFLATE, TILE_256,
	 &ret))
	return ret;
    ret = -380;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_DEFLATE, TILE_512,
	 &ret))
	return ret;
    ret = -390;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_DEFLATE, TILE_1024,
	 &ret))
	return ret;
	
#ifndef OMIT_LZMA	/* only if LZMA is enabled */
    ret = -410;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_LZMA, TILE_256, &ret))
	return ret;
    ret = -420;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_LZMA, TILE_512, &ret))
	return ret;
    ret = -420;
    if (!test_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_LZMA, TILE_1024,
	 &ret))
	return ret;
#endif	/* end LZMA conditional */

/* PALETTE tests */
    ret = -400;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -420;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -440;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -500;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -520;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -540;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -560;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_DEFLATE, TILE_256, &ret))
	return ret;
    ret = -580;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_DEFLATE, TILE_512, &ret))
	return ret;
    ret = -600;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_DEFLATE, TILE_1024,
	 &ret))
	return ret;
	
#ifndef OMIT_LZMA	/* only if LZMA is enabled */
    ret = -620;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_LZMA, TILE_256, &ret))
	return ret;
    ret = -640;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_LZMA, TILE_512, &ret))
	return ret;
    ret = -660;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_LZMA, TILE_1024, &ret))
	return ret;
#endif	/* end LZMA conditional */

/* dropping all MONOCHROME Coverages */
    ret = -170;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -190;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE, TILE_1024,
	 &ret))
	return ret;
    ret = -270;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -290;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -370;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_CCITTFAX4, TILE_256,
	 &ret))
	return ret;
    ret = -380;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_CCITTFAX4, TILE_512,
	 &ret))
	return ret;
    ret = -390;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_CCITTFAX4, TILE_1024,
	 &ret))
	return ret;
    ret = -400;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_DEFLATE, TILE_256,
	 &ret))
	return ret;
    ret = -410;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_DEFLATE, TILE_512,
	 &ret))
	return ret;
    ret = -420;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_DEFLATE, TILE_1024,
	 &ret))
	return ret;
	
#ifndef OMIT_LZMA	/* only if LZMA is enabled */
    ret = -430;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_LZMA, TILE_256, &ret))
	return ret;
    ret = -440;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_LZMA, TILE_512, &ret))
	return ret;
    ret = -450;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_LZMA, TILE_1024,
	 &ret))
	return ret;
#endif	/* end LZMA conditional */

/* dropping all PALETTE Coverages */
    ret = -470;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -480;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -490;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -570;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -580;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -590;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -600;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_DEFLATE, TILE_256, &ret))
	return ret;
    ret = -610;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_DEFLATE, TILE_512, &ret))
	return ret;
    ret = -620;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_DEFLATE, TILE_1024,
	 &ret))
	return ret;
	
#ifndef OMIT_LZMA	/* only if LZMA is enabled */
    ret = -630;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_LZMA, TILE_256, &ret))
	return ret;
    ret = -640;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_LZMA, TILE_512, &ret))
	return ret;
    ret = -650;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_LZMA, TILE_1024, &ret))
	return ret;
#endif	/* end LZMA conditional */

/* closing the DB */
    sqlite3_close (db_handle);
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
