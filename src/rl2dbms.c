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

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#include <spatialite/gaiaaux.h>

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

static int
insert_into_raster_coverages (sqlite3 * handle, const char *coverage,
			      unsigned char sample, unsigned char pixel,
			      unsigned char num_bands,
			      unsigned char compression, int quality,
			      unsigned short tile_width,
			      unsigned short tile_height, int srid,
			      double x_res, double y_res, unsigned char *blob,
			      int blob_sz, unsigned char *blob_no_data,
			      int blob_no_data_sz)
{
/* inserting into "raster_coverages" */
    int ret;
    char *sql;
    sqlite3_stmt *stmt;
    const char *xsample = "UNKNOWN";
    const char *xpixel = "UNKNOWN";
    const char *xcompression = "UNKNOWN";

    sql = "INSERT INTO raster_coverages (coverage_name, sample_type, "
	"pixel_type, num_bands, compression, quality, tile_width, "
	"tile_height, horz_resolution, vert_resolution, srid, "
	"nodata_pixel, palette) VALUES (Lower(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  return 0;
      }
    switch (sample)
      {
      case RL2_SAMPLE_1_BIT:
	  xsample = "1-BIT";
	  break;
      case RL2_SAMPLE_2_BIT:
	  xsample = "2-BIT";
	  break;
      case RL2_SAMPLE_4_BIT:
	  xsample = "4-BIT";
	  break;
      case RL2_SAMPLE_INT8:
	  xsample = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  xsample = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  xsample = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  xsample = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  xsample = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  xsample = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  xsample = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  xsample = "DOUBLE";
	  break;
      };
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  xpixel = "MONOCHROME";
	  break;
      case RL2_PIXEL_PALETTE:
	  xpixel = "PALETTE";
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  xpixel = "GRAYSCALE";
	  break;
      case RL2_PIXEL_RGB:
	  xpixel = "RGB";
	  break;
      case RL2_PIXEL_MULTIBAND:
	  xpixel = "MULTIBAND";
	  break;
      case RL2_PIXEL_DATAGRID:
	  xpixel = "DATAGRID";
	  break;
      };
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  xcompression = "NONE";
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  xcompression = "DEFLATE";
	  break;
      case RL2_COMPRESSION_LZMA:
	  xcompression = "LZMA";
	  break;
      case RL2_COMPRESSION_PNG:
	  xcompression = "PNG";
	  break;
      case RL2_COMPRESSION_JPEG:
	  xcompression = "JPEG";
	  break;
      case RL2_COMPRESSION_LOSSY_WEBP:
	  xcompression = "LOSSY_WEBP";
	  break;
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  xcompression = "LOSSLESS_WEBP";
	  break;
      case RL2_COMPRESSION_CCITTFAX4:
	  xcompression = "CCITTFAX4";
	  break;
      };
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, xsample, strlen (xsample), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, xpixel, strlen (xpixel), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, num_bands);
    sqlite3_bind_text (stmt, 5, xcompression, strlen (xcompression),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt, 6, quality);
    sqlite3_bind_int (stmt, 7, tile_width);
    sqlite3_bind_int (stmt, 8, tile_height);
    sqlite3_bind_double (stmt, 9, x_res);
    sqlite3_bind_double (stmt, 10, y_res);
    sqlite3_bind_int (stmt, 11, srid);
    if (blob_no_data == NULL)
	sqlite3_bind_null (stmt, 12);
    else
	sqlite3_bind_blob (stmt, 12, blob_no_data, blob_no_data_sz, free);
    if (blob == NULL)
	sqlite3_bind_null (stmt, 13);
    else
	sqlite3_bind_blob (stmt, 13, blob, blob_sz, free);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	goto coverage_registered;
    fprintf (stderr,
	     "sqlite3_step() error: INSERT INTO raster_coverages \"%s\"\n",
	     sqlite3_errmsg (handle));
    sqlite3_finalize (stmt);
    return 0;
  coverage_registered:
    sqlite3_finalize (stmt);
    return 1;
}

static int
create_levels (sqlite3 * handle, const char *coverage)
{
/* creating the LEVELS table */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *xcoverage;
    char *xxcoverage;

    xcoverage = sqlite3_mprintf ("%s_levels", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\tpyramid_level INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tx_resolution_1_1 DOUBLE NOT NULL,\n"
			   "\ty_resolution_1_1 DOUBLE NOT NULL,\n"
			   "\tx_resolution_1_2 DOUBLE,\n"
			   "\ty_resolution_1_2 DOUBLE,\n"
			   "\tx_resolution_1_4 DOUBLE,\n"
			   "\ty_resolution_1_4 DOUBLE,\n"
			   "\tx_resolution_1_8 DOUBLE,\n"
			   "\ty_resolution_1_8 DOUBLE)\n", xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s_levels\" error: %s\n", xxcoverage,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);
    return 1;
}

static int
create_sections (sqlite3 * handle, const char *coverage, int srid)
{
/* creating the SECTIONS table */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *xcoverage;
    char *xxcoverage;
    char *xindex;
    char *xxindex;
    char *xtrigger;
    char *xxtrigger;

/* creating the SECTIONS table */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\tsection_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tsection_name TEXT NOT NULL,\n"
			   "\twidth INTEGER NOT NULL,\n"
			   "\theight INTEGER NOT NULL,\n"
			   "\tfile_path TEXT,\n"
			   "\tstatistics BLOB)", xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* adding the safeguard Triggers */
    xtrigger = sqlite3_mprintf ("%s_sections_statistics_insert", coverage);
    xxtrigger = gaiaDoubleQuotedSql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE INSERT ON %Q\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'insert on %s violates constraint: "
			   "invalid statistics')\nWHERE NEW.statistics IS NOT NULL AND "
			   "IsValidRasterStatistics(%Q, NEW.statistics) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage);
    sqlite3_free (xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);
    xtrigger = sqlite3_mprintf ("%s_sections_statistics_update", coverage);
    xxtrigger = gaiaDoubleQuotedSql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE UPDATE OF 'statistics' ON %Q"
			   "\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT, 'update on %s violates constraint: "
			   "invalid statistics')\nWHERE NEW.statistics IS NOT NULL AND "
			   "IsValidRasterStatistics(%Q, NEW.statistics) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage);
    sqlite3_free (xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);

/* creating the SECTIONS geometry */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("SELECT AddGeometryColumn("
			   "%Q, 'geometry', %d, 'POLYGON', 'XY')", xcoverage,
			   srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn \"%s\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the SECTIONS spatial index */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex("
			   "%Q, 'geometry')", xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateSpatialIndex \"%s\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the SECTIONS index by name */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    xindex = sqlite3_mprintf ("idx_%s_sections", coverage);
    xxindex = gaiaDoubleQuotedSql (xindex);
    sqlite3_free (xindex);
    sql =
	sqlite3_mprintf ("CREATE UNIQUE INDEX \"%s\" ON \"%s\" (section_name)",
			 xxindex, xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX \"%s\" error: %s\n", xxindex, sql_err);
	  sqlite3_free (sql_err);
	  free (xxindex);
	  return 0;
      }
    free (xxindex);
    return 1;
}

static int
create_tiles (sqlite3 * handle, const char *coverage, int srid)
{
/* creating the TILES table */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *xcoverage;
    char *xxcoverage;
    char *xindex;
    char *xxindex;
    char *xfk;
    char *xxfk;
    char *xmother;
    char *xxmother;
    char *xfk2;
    char *xxfk2;
    char *xmother2;
    char *xxmother2;
    char *xtrigger;
    char *xxtrigger;
    char *xtiles;
    char *xxtiles;

    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    xmother = sqlite3_mprintf ("%s_sections", coverage);
    xxmother = gaiaDoubleQuotedSql (xmother);
    sqlite3_free (xmother);
    xfk = sqlite3_mprintf ("fk_%s_tiles_section", coverage);
    xxfk = gaiaDoubleQuotedSql (xfk);
    sqlite3_free (xfk);
    xmother2 = sqlite3_mprintf ("%s_levels", coverage);
    xxmother2 = gaiaDoubleQuotedSql (xmother2);
    sqlite3_free (xmother2);
    xfk2 = sqlite3_mprintf ("fk_%s_tiles_level", coverage);
    xxfk2 = gaiaDoubleQuotedSql (xfk2);
    sqlite3_free (xfk2);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\ttile_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tpyramid_level INTEGER NOT NULL,\n"
			   "\tsection_id INTEGER NOT NULL,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (section_id) "
			   "REFERENCES \"%s\" (section_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (pyramid_level) "
			   "REFERENCES \"%s\" (pyramid_level) ON DELETE CASCADE)",
			   xxcoverage, xxfk, xxmother, xxfk2, xxmother2);
    free (xxfk);
    free (xxmother);
    free (xxfk2);
    free (xxmother2);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s_tiles\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* creating the TILES geometry */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    sql = sqlite3_mprintf ("SELECT AddGeometryColumn("
			   "%Q, 'geometry', %d, 'POLYGON', 'XY')", xcoverage,
			   srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn \"%s_tiles\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the TILES spatial Index */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex("
			   "%Q, 'geometry')", xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateSpatialIndex \"%s_tiles\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the TILES index by section */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    xindex = sqlite3_mprintf ("idx_%s_tiles", coverage);
    xxindex = gaiaDoubleQuotedSql (xindex);
    sqlite3_free (xindex);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (section_id)", xxindex,
			 xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX \"%s\" error: %s\n", xxindex, sql_err);
	  sqlite3_free (sql_err);
	  free (xxindex);
	  return 0;
      }
    free (xxindex);

/* creating the TILE_DATA table */
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    xmother = sqlite3_mprintf ("%s_tiles", coverage);
    xxmother = gaiaDoubleQuotedSql (xmother);
    sqlite3_free (xmother);
    xfk = sqlite3_mprintf ("fk_%s_tile_data", coverage);
    xxfk = gaiaDoubleQuotedSql (xfk);
    sqlite3_free (xfk);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\ttile_id INTEGER NOT NULL PRIMARY KEY,\n"
			   "\ttile_data_odd BLOB NOT NULL,\n"
			   "\ttile_data_even BLOB,\n"
			   "CONSTRAINT \"%s\" FOREIGN KEY (tile_id) "
			   "REFERENCES \"%s\" (tile_id) ON DELETE CASCADE)",
			   xxcoverage, xxfk, xxmother);
    free (xxfk);
    free (xxmother);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s_tile_data\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* adding the safeguard Triggers */
    xtrigger = sqlite3_mprintf ("%s_tile_data_insert", coverage);
    xxtrigger = gaiaDoubleQuotedSql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage);
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = gaiaDoubleQuotedSql (xtiles);
    sqlite3_free (xtiles);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE INSERT ON %Q\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'insert on %s violates constraint: "
			   "invalid tile_data')\nWHERE IsValidRasterTile(%Q, "
			   "(SELECT t.pyramid_level FROM \"%s\" AS t WHERE t.tile_id = NEW.tile_id), "
			   "NEW.tile_data_odd, NEW.tile_data_even) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage, xxtiles);
    sqlite3_free (xcoverage);
    free (xxtiles);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);
    xtrigger = sqlite3_mprintf ("%s_tile_data_update", coverage);
    xxtrigger = gaiaDoubleQuotedSql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage);
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = gaiaDoubleQuotedSql (xtiles);
    sqlite3_free (xtiles);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE UPDATE ON %Q\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT, 'update on %s violates constraint: "
			   "invalid tile_data')\nWHERE IsValidRasterTile(%Q, "
			   "(SELECT t.pyramid_level FROM \"%s\" AS t WHERE t.tile_id = NEW.tile_id), "
			   "NEW.tile_data_odd, NEW.tile_data_even) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage, xxtiles);
    sqlite3_free (xcoverage);
    free (xxtiles);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);
    return 1;
}

RL2_DECLARE int
rl2_create_dbms_coverage (sqlite3 * handle, const char *coverage,
			  unsigned char sample, unsigned char pixel,
			  unsigned char num_bands, unsigned char compression,
			  int quality, unsigned short tile_width,
			  unsigned short tile_height, int srid, double x_res,
			  double y_res, rl2PixelPtr no_data,
			  rl2PalettePtr palette)
{
/* creating a DBMS-based Coverage */
    unsigned char *blob = NULL;
    int blob_size = 0;
    unsigned char *blob_no_data = NULL;
    int blob_no_data_sz = 0;
    if (pixel == RL2_PIXEL_PALETTE)
      {
	  /* installing a default (empty) Palette */
	  if (rl2_serialize_dbms_palette (palette, &blob, &blob_size) != RL2_OK)
	      goto error;
      }
    if (no_data != NULL)
      {
	  if (rl2_serialize_dbms_pixel
	      (no_data, &blob_no_data, &blob_no_data_sz) != RL2_OK)
	      goto error;
      }
    if (!insert_into_raster_coverages
	(handle, coverage, sample, pixel, num_bands, compression, quality,
	 tile_width, tile_height, srid, x_res, y_res, blob, blob_size,
	 blob_no_data, blob_no_data_sz))
	goto error;
    if (!create_levels (handle, coverage))
	goto error;
    if (!create_sections (handle, coverage, srid))
	goto error;
    if (!create_tiles (handle, coverage, srid))
	goto error;
    return RL2_OK;
  error:
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_dbms_section_id (sqlite3 * handle, const char *coverage,
			 const char *section, sqlite3_int64 * section_id)
{
/* retrieving a Section ID by its name */
    int ret;
    char *sql;
    char *table;
    char *xtable;
    int found = 0;
    sqlite3_stmt *stmt = NULL;

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT section_id FROM \"%s\" WHERE section_name = ?",
			 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT section_name SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* querying the section */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, section, strlen (section), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		*section_id = sqlite3_column_int64 (stmt, 0);
		found++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_name; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (found == 1)
	return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_delete_dbms_section (sqlite3 * handle, const char *coverage,
			 sqlite3_int64 section_id)
{
/* deleting a Raster Section */
    int ret;
    char *sql;
    rl2CoveragePtr cvg = NULL;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt = NULL;

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("DELETE sections SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

/* DELETing the section */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, section_id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "DELETE sections; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_finalize (stmt);

    rl2_destroy_coverage (cvg);
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_drop_dbms_coverage (sqlite3 * handle, const char *coverage)
{
/* dropping a Raster Coverage */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *table;
    char *xtable;

/* disabling the SECTIONS spatial index */
    xtable = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex("
			   "%Q, 'geometry')", xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DisableSpatialIndex \"%s\" error: %s\n", xtable,
		   sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xtable);
	  goto error;
      }
    sqlite3_free (xtable);

/* dropping the SECTIONS spatial index */
    table = sqlite3_mprintf ("idx_%s_sections_geometry", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* disabling the TILES spatial index */
    xtable = sqlite3_mprintf ("%s_tiles", coverage);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex("
			   "%Q, 'geometry')", xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DisableSpatialIndex \"%s\" error: %s\n", xtable,
		   sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xtable);
	  goto error;
      }
    sqlite3_free (xtable);

/* dropping the TILES spatial index */
    table = sqlite3_mprintf ("idx_%s_tiles_geometry", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* dropping the TILE_DATA table */
    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* deleting the TILES Geometry definition */
    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM geometry_columns "
			   "WHERE Lower(f_table_name) = Lower(%Q)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE TilesGeometry \"%s\" error: %s\n",
		   coverage, sql_err);
	  sqlite3_free (sql_err);
	  goto error;
      }

/* deleting the SECTIONS Geometry definition */
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("DELETE FROM geometry_columns "
			   "WHERE Lower(f_table_name) = Lower(%Q)", xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE SectionsGeometry \"%s\" error: %s\n",
		   coverage, sql_err);
	  sqlite3_free (sql_err);
	  goto error;
      }

/* dropping the TILES table */
    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* dropping the SECTIONS table */
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* dropping the LEVELS table */
    table = sqlite3_mprintf ("%s_levels", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* deleting the Raster Coverage definition */
    sql = sqlite3_mprintf ("DELETE FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE raster_coverage \"%s\" error: %s\n",
		   coverage, sql_err);
	  sqlite3_free (sql_err);
	  goto error;
      }
    return RL2_OK;

  error:
    return RL2_ERROR;
}

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

RL2_DECLARE void
rl2_prime_void_tile_palette (void *pixels, unsigned short width,
			     unsigned short height, rl2PalettePtr palette)
{
/* priming a void tile buffer (PALETTE) */
    int row;
    int col;
    int j;
    int index = -1;
    unsigned char *p = pixels;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;

    if (plt == NULL)
	;
    else
      {
	  /* searching for WHITE */
	  rl2PrivPaletteEntryPtr entry;
	  for (j = 0; j < plt->nEntries; j++)
	    {
		entry = plt->entries + j;
		if (entry->red == 255 && entry->green == 255
		    && entry->blue == 255)
		  {
		      index = j;
		      break;
		  }
	    }
	  if (index < 0)
	    {
		/* searching for BLACK */
		for (j = 0; j < plt->nEntries; j++)
		  {
		      entry = plt->entries + j;
		      if (entry->red == 0 && entry->green == 0
			  && entry->blue == 0)
			{
			    index = j;
			    break;
			}
		  }
	    }
      }
    if (index < 0)
      {
	  /* not WHITE neither BLACK are defined */
	  /* defaulting to first palette entry */
	  index = 0;
      }

    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = index;
      }
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
    rl2PixelPtr no_data = NULL;
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
		int ok_nodata = 1;
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
		      if (strcasecmp (value, "CCITTFAX4") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_CCITTFAX4;
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
		      x_res = sqlite3_column_double (stmt, 7);
		      ok_x_res = 1;
		  }
		if (sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      y_res = sqlite3_column_double (stmt, 8);
		      ok_y_res = 1;
		  }
		if (sqlite3_column_type (stmt, 9) == SQLITE_INTEGER)
		  {
		      srid = sqlite3_column_int (stmt, 9);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 10) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt, 10);
		      int blob_sz = sqlite3_column_bytes (stmt, 10);
		      no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
		      if (no_data == NULL)
			  ok_nodata = 0;
		  }
		if (ok_sample && ok_pixel && ok_num_bands && ok_compression
		    && ok_quality && ok_tile_width && ok_tile_height && ok_x_res
		    && ok_y_res && ok_srid && ok_nodata)
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
			     quality, tile_width, tile_height, no_data);
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
		       unsigned short height, unsigned char num_bands,
		       rl2PixelPtr no_data)
{
/* preparing an empty/void UINT8 raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned short x;
    unsigned short y;
    unsigned char b;
    unsigned char *p = buffer;
    int has_nodata = 0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->nBands == num_bands)
	      has_nodata = 1;
      }
    if (!has_nodata)
      {
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      for (b = 0; b < num_bands; b++)
			  *p++ = 0;
		  }
	    }
      }
    else
      {
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      for (b = 0; b < num_bands; b++)
			{
			    rl2PrivSamplePtr sample = pxl->Samples + b;
			    *p++ = sample->uint8;
			}
		  }
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

RL2_PRIVATE void
void_raw_buffer (unsigned char *buffer, unsigned short width,
		 unsigned short height, unsigned char sample_type,
		 unsigned char num_bands, rl2PixelPtr no_data)
{
/* preparing an empty/void buffer */
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
				 num_bands, no_data);
	  break;
      };
}

RL2_PRIVATE void
void_raw_buffer_palette (unsigned char *buffer, unsigned short width,
			 unsigned short height, rl2PalettePtr palette,
			 rl2PixelPtr no_data)
{
/* preparing an empty/void buffer (PALETTE) */
    int row;
    int col;
    int j;
    int index = -1;
    unsigned char *p = buffer;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;

    if (plt == NULL)
	;
    else
      {
	  /* searching for WHITE */
	  rl2PrivPaletteEntryPtr entry;
	  for (j = 0; j < plt->nEntries; j++)
	    {
		entry = plt->entries + j;
		if (entry->red == 255 && entry->green == 255
		    && entry->blue == 255)
		  {
		      index = j;
		      break;
		  }
	    }
	  if (index < 0)
	    {
		/* searching for BLACK */
		for (j = 0; j < plt->nEntries; j++)
		  {
		      entry = plt->entries + j;
		      if (entry->red == 0 && entry->green == 0
			  && entry->blue == 0)
			{
			    index = j;
			    break;
			}
		  }
	    }
      }
    if (index < 0)
      {
	  /* not WHITE neither BLACK are defined */
	  /* defaulting to first palette entry */
	  index = 0;
      }

    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = index;
      }
}

static void
copy_int8_raw_pixels (const char *buffer, const unsigned char *mask,
		      char *outbuf, unsigned short width,
		      unsigned short height, double x_res, double y_res,
		      double minx, double maxy, double tile_minx,
		      double tile_maxy, unsigned short tile_width,
		      unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const char *p_in = buffer;
    const unsigned char *p_msk = mask;
    char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT8)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const char *p_save = p_in;
		      char sample = 0;
		      rl2_get_pixel_sample_int8 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_uint8_raw_pixels (const unsigned char *buffer, const unsigned char *mask,
		       unsigned char *outbuf, unsigned short width,
		       unsigned short height, unsigned char num_bands,
		       double x_res, double y_res, double minx, double maxy,
		       double tile_minx, double tile_maxy,
		       unsigned short tile_width, unsigned short tile_height,
		       rl2PixelPtr no_data)
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
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != num_bands)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	      || sample_type == RL2_SAMPLE_4_BIT
	      || sample_type == RL2_SAMPLE_UINT8)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * num_bands) + (out_x * num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      for (b = 0; b < num_bands; b++)
			{
			    if (transparent)
			      {
				  /* skipping a transparent pixel */
				  p_out++;
				  p_in++;
			      }
			    else
				*p_out++ = *p_in++;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    switch (sample_type)
			      {
			      case RL2_SAMPLE_1_BIT:
				  rl2_get_pixel_sample_1bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_2_BIT:
				  rl2_get_pixel_sample_2bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_4_BIT:
				  rl2_get_pixel_sample_4bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_UINT8:
				  rl2_get_pixel_sample_uint8 (no_data, b,
							      &sample);
				  break;
			      };
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    for (b = 0; b < num_bands; b++)
				*p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (b = 0; b < num_bands; b++)
				p_out++;
			}
		  }
	    }
      }
}

static void
copy_int16_raw_pixels (const short *buffer, const unsigned char *mask,
		       short *outbuf, unsigned short width,
		       unsigned short height, double x_res, double y_res,
		       double minx, double maxy, double tile_minx,
		       double tile_maxy, unsigned short tile_width,
		       unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const short *p_in = buffer;
    const unsigned char *p_msk = mask;
    short *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT16)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const short *p_save = p_in;
		      short sample = 0;
		      rl2_get_pixel_sample_int16 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_uint16_raw_pixels (const unsigned short *buffer, const unsigned char *mask,
			unsigned short *outbuf, unsigned short width,
			unsigned short height, unsigned char num_bands,
			double x_res, double y_res, double minx, double maxy,
			double tile_minx, double tile_maxy,
			unsigned short tile_width, unsigned short tile_height,
			rl2PixelPtr no_data)
{
/* copying UINT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned short *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != num_bands)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_UINT16)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * num_bands) + (out_x * num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      for (b = 0; b < num_bands; b++)
			{
			    if (transparent)
			      {
				  /* skipping a transparent pixel */
				  p_out++;
				  p_in++;
			      }
			    else
				*p_out++ = *p_in++;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    for (b = 0; b < num_bands; b++)
				*p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (b = 0; b < num_bands; b++)
				p_out++;
			}
		  }
	    }
      }
}

static void
copy_int32_raw_pixels (const int *buffer, const unsigned char *mask,
		       int *outbuf, unsigned short width,
		       unsigned short height, double x_res, double y_res,
		       double minx, double maxy, double tile_minx,
		       double tile_maxy, unsigned short tile_width,
		       unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT32 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const int *p_in = buffer;
    const unsigned char *p_msk = mask;
    int *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT32)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const int *p_save = p_in;
		      int sample = 0;
		      rl2_get_pixel_sample_int32 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_uint32_raw_pixels (const unsigned int *buffer, const unsigned char *mask,
			unsigned int *outbuf, unsigned short width,
			unsigned short height, double x_res, double y_res,
			double minx, double maxy, double tile_minx,
			double tile_maxy, unsigned short tile_width,
			unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned int *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned int *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_UINT32)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned int *p_save = p_in;
		      unsigned int sample = 0;
		      rl2_get_pixel_sample_uint32 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_float_raw_pixels (const float *buffer, const unsigned char *mask,
		       float *outbuf, unsigned short width,
		       unsigned short height, double x_res, double y_res,
		       double minx, double maxy, double tile_minx,
		       double tile_maxy, unsigned short tile_width,
		       unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying FLOAT raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const float *p_in = buffer;
    const unsigned char *p_msk = mask;
    float *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_FLOAT)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const float *p_save = p_in;
		      float sample = 0;
		      rl2_get_pixel_sample_float (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_double_raw_pixels (const double *buffer, const unsigned char *mask,
			double *outbuf, unsigned short width,
			unsigned short height, double x_res, double y_res,
			double minx, double maxy, double tile_minx,
			double tile_maxy, unsigned short tile_width,
			unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying DOUBLE raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const double *p_in = buffer;
    const unsigned char *p_msk = mask;
    double *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_DOUBLE)
	      ;
	  else
	      ignore_no_data = 1;
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const double *p_save = p_in;
		      double sample = 0;
		      rl2_get_pixel_sample_double (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static int
copy_raw_pixels (rl2RasterPtr raster, unsigned char *outbuf,
		 unsigned short width,
		 unsigned short height, unsigned char sample_type,
		 unsigned char num_bands, double x_res, double y_res,
		 double minx, double maxy, double tile_minx, double tile_maxy,
		 rl2PixelPtr no_data)
{
/* copying raw pixels into the output buffer */
    unsigned short tile_width;
    unsigned short tile_height;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  copy_int8_raw_pixels ((const char *) (rst->rasterBuffer),
				(const unsigned char *) (rst->maskBuffer),
				(char *) outbuf, width, height,
				x_res, y_res, minx, maxy, tile_minx,
				tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_INT16:
	  copy_int16_raw_pixels ((const short *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (short *) outbuf, width, height,
				 x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_raw_pixels ((const unsigned short *) (rst->rasterBuffer),
				  (const unsigned char *) (rst->maskBuffer),
				  (unsigned short *) outbuf, width, height,
				  num_bands, x_res, y_res, minx, maxy,
				  tile_minx, tile_maxy, tile_width,
				  tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_INT32:
	  copy_int32_raw_pixels ((const int *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (int *) outbuf, width, height,
				 x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_raw_pixels ((const unsigned int *) (rst->rasterBuffer),
				  (const unsigned char *) (rst->maskBuffer),
				  (unsigned int *) outbuf, width, height,
				  x_res, y_res, minx, maxy,
				  tile_minx, tile_maxy, tile_width,
				  tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_FLOAT:
	  copy_float_raw_pixels ((const float *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (float *) outbuf, width, height,
				 x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_raw_pixels ((const double *) (rst->rasterBuffer),
				  (const unsigned char *) (rst->maskBuffer),
				  (double *) outbuf, width, height,
				  x_res, y_res, minx, maxy,
				  tile_minx, tile_maxy, tile_width,
				  tile_height, no_data);
	  return 1;
      default:
	  copy_uint8_raw_pixels ((const unsigned char *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (unsigned char *) outbuf, width, height,
				 num_bands, x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      };

    return 0;
}

static int
load_dbms_tiles_common (sqlite3 * handle, sqlite3_stmt * stmt_tiles,
			sqlite3_stmt * stmt_data, unsigned char *outbuf,
			unsigned short width,
			unsigned short height, unsigned char sample_type,
			unsigned char num_bands, double x_res, double y_res,
			double minx, double maxy,
			int scale, rl2PalettePtr palette, rl2PixelPtr no_data)
{
/* retrieving a full image from DBMS tiles */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr plt = NULL;
    int ret;

/* querying the tiles */
    while (1)
      {
	  ret = sqlite3_step (stmt_tiles);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob_odd = NULL;
		int blob_odd_sz = 0;
		const unsigned char *blob_even = NULL;
		int blob_even_sz = 0;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_tiles, 0);
		double tile_minx = sqlite3_column_double (stmt_tiles, 1);
		double tile_maxy = sqlite3_column_double (stmt_tiles, 2);

		/* retrieving tile raw data from BLOBs */
		sqlite3_reset (stmt_data);
		sqlite3_clear_bindings (stmt_data);
		sqlite3_bind_int64 (stmt_data, 1, tile_id);
		ret = sqlite3_step (stmt_data);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_type (stmt_data, 0) == SQLITE_BLOB)
			{
			    blob_odd = sqlite3_column_blob (stmt_data, 0);
			    blob_odd_sz = sqlite3_column_bytes (stmt_data, 0);
			}
		      if (scale == RL2_SCALE_1)
			{
			    if (sqlite3_column_type (stmt_data, 1) ==
				SQLITE_BLOB)
			      {
				  blob_even =
				      sqlite3_column_blob (stmt_data, 1);
				  blob_even_sz =
				      sqlite3_column_bytes (stmt_data, 1);
			      }
			}
		  }
		else
		  {
		      fprintf (stderr,
			       "SELECT tiles data; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
		plt = rl2_clone_palette (palette);
		raster =
		    rl2_raster_decode (scale, blob_odd, blob_odd_sz, blob_even,
				       blob_even_sz, plt);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      goto error;
		  }
		plt = NULL;
		if (!copy_raw_pixels
		    (raster, outbuf, width, height, sample_type,
		     num_bands, x_res, y_res, minx, maxy, tile_minx, tile_maxy,
		     no_data))
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
    if (plt != NULL)
	rl2_destroy_palette (plt);
    return 0;
}

RL2_PRIVATE int
load_dbms_tiles (sqlite3 * handle, sqlite3_stmt * stmt_tiles,
		 sqlite3_stmt * stmt_data, unsigned char *outbuf,
		 unsigned short width,
		 unsigned short height, unsigned char sample_type,
		 unsigned char num_bands, double x_res, double y_res,
		 double minx, double miny, double maxx, double maxy, int level,
		 int scale, rl2PalettePtr palette, rl2PixelPtr no_data)
{
/* binding the query args */
    sqlite3_reset (stmt_tiles);
    sqlite3_clear_bindings (stmt_tiles);
    sqlite3_bind_int (stmt_tiles, 1, level);
    sqlite3_bind_double (stmt_tiles, 2, minx);
    sqlite3_bind_double (stmt_tiles, 3, miny);
    sqlite3_bind_double (stmt_tiles, 4, maxx);
    sqlite3_bind_double (stmt_tiles, 5, maxy);

    if (!load_dbms_tiles_common
	(handle, stmt_tiles, stmt_data, outbuf, width, height,
	 sample_type, num_bands, x_res, y_res, minx, maxy, scale,
	 palette, no_data))
	return 0;
    return 1;
}

RL2_PRIVATE int
load_dbms_tiles_section (sqlite3 * handle, sqlite3_int64 section_id,
			 sqlite3_stmt * stmt_tiles, sqlite3_stmt * stmt_data,
			 unsigned char *outbuf,
			 unsigned short width, unsigned short height,
			 unsigned char sample_type, unsigned char num_bands,
			 double x_res, double y_res, double minx, double maxy,
			 int scale, rl2PalettePtr palette, rl2PixelPtr no_data)
{
/* binding the query args */
    sqlite3_reset (stmt_tiles);
    sqlite3_clear_bindings (stmt_tiles);
    sqlite3_bind_int (stmt_tiles, 1, section_id);

    if (!load_dbms_tiles_common
	(handle, stmt_tiles, stmt_data, outbuf, width, height,
	 sample_type, num_bands, x_res, y_res, minx, maxy, scale,
	 palette, no_data))
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_find_matching_resolution (sqlite3 * handle, rl2CoveragePtr cvg,
			      double *x_res, double *y_res,
			      unsigned char *level, unsigned char *scale)
{
/* attempting to identify the corresponding resolution level */
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
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

    if (coverage == NULL)
	return RL2_ERROR;
    if (coverage->coverageName == NULL)
	return RL2_ERROR;

    xcoverage = sqlite3_mprintf ("%s_levels", coverage->coverageName);
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
			    x_scale = RL2_SCALE_1;
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
			    x_scale = RL2_SCALE_2;
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
			    x_scale = RL2_SCALE_4;
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
			    x_scale = RL2_SCALE_8;
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
			 int *buf_size, rl2PalettePtr * palette,
			 unsigned char out_pixel)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    rl2PalettePtr plt = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PixelPtr kill_no_data = NULL;
    const char *coverage;
    unsigned char level;
    unsigned char scale;
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
    sqlite3_stmt *stmt_tiles = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;

    if (cvg == NULL || handle == NULL)
	goto error;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	goto error;
    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	goto error;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;

    if (pixel_type == RL2_PIXEL_MONOCHROME && out_pixel == RL2_PIXEL_GRAYSCALE)
      {
	  /* Pyramid tiles MONOCHROME */
	  rl2PixelPtr nd = NULL;
	  nd = rl2_get_coverage_no_data (cvg);
	  if (nd != NULL)
	    {
		/* creating a Grayscale NoData pixel */
		rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) nd;
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		no_data = rl2_create_pixel (RL2_SAMPLE_UINT8,
					    RL2_PIXEL_GRAYSCALE, 1);
		kill_no_data = no_data;
		if (sample->uint8 == 0)
		    rl2_set_pixel_sample_uint8 (no_data,
						RL2_GRAYSCALE_BAND, 255);
		else
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 0);
	    }
	  sample_type = RL2_SAMPLE_UINT8;
	  pixel_type = RL2_PIXEL_GRAYSCALE;
	  num_bands = 1;
      }
    else if (pixel_type == RL2_PIXEL_PALETTE && out_pixel == RL2_PIXEL_RGB)
      {
	  /* Pyramid tiles RGB */
	  rl2PixelPtr nd = NULL;
	  /*
	     nd = rl2_get_coverage_no_data (cvg);
	     if (nd != NULL)
	     {
	     /* creating a Grayscale NoData pixel /
	     rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) nd;
	     rl2PrivSamplePtr sample = pxl->Samples + 0;
	     no_data = rl2_create_pixel (RL2_SAMPLE_UINT8,
	     RL2_PIXEL_GRAYSCALE, 1);
	     kill_no_data = no_data;
	     if (sample->uint8 == 0)
	     rl2_set_pixel_sample_uint8 (no_data,
	     RL2_GRAYSCALE_BAND, 255);
	     else
	     rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 0);
	     }
	   */
	  sample_type = RL2_SAMPLE_UINT8;
	  pixel_type = RL2_PIXEL_RGB;
	  num_bands = 3;
      }
    else
      {
	  if (pixel_type == RL2_PIXEL_PALETTE)
	    {
		/* attempting to retrieve the Coverage's Palette */
		plt = rl2_get_dbms_palette (handle, coverage);
		if (plt == NULL)
		    goto error;
	    }
	  no_data = rl2_get_coverage_no_data (cvg);
      }

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

/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = gaiaDoubleQuotedSql (xtiles);
    sql =
	sqlite3_mprintf ("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry) "
			 "FROM \"%s\" "
			 "WHERE pyramid_level = ? AND ROWID IN ( "
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
			 "AND search_frame = BuildMBR(?, ?, ?, ?))", xxtiles,
			 xtiles);
    sqlite3_free (xtiles);
    free (xxtiles);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tiles, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT raw tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    if (scale == RL2_SCALE_1)
      {
	  /* preparing the data SQL query - both ODD and EVEN */
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = gaiaDoubleQuotedSql (xdata);
	  sqlite3_free (xdata);
	  sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
				 "FROM \"%s\" WHERE tile_id = ?", xxdata);
	  free (xxdata);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("SELECT raw tiles data(2) SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }
    else
      {
	  /* preparing the data SQL query - only ODD */
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = gaiaDoubleQuotedSql (xdata);
	  sqlite3_free (xdata);
	  sql = sqlite3_mprintf ("SELECT tile_data_odd "
				 "FROM \"%s\" WHERE tile_id = ?", xxdata);
	  free (xxdata);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("SELECT raw tiles data(1) SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }

/* preparing a raw pixels buffer */
    if (pixel_type == RL2_PIXEL_PALETTE)
	void_raw_buffer_palette (bufpix, width, height, plt, no_data);
    else
	void_raw_buffer (bufpix, width, height, sample_type, num_bands,
			 no_data);
    if (!load_dbms_tiles
	(handle, stmt_tiles, stmt_data, bufpix, width, height, sample_type,
	 num_bands, xx_res, yy_res, minx, miny, maxx, maxy, level, scale, plt,
	 no_data))
	goto error;
    if (kill_no_data != NULL)
	rl2_destroy_pixel (kill_no_data);
    sqlite3_finalize (stmt_tiles);
    sqlite3_finalize (stmt_data);
    *buffer = bufpix;
    *buf_size = bufpix_size;
    *palette = plt;
    return RL2_OK;

  error:
    if (stmt_tiles != NULL)
	sqlite3_finalize (stmt_tiles);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (bufpix != NULL)
	free (bufpix);
    if (kill_no_data != NULL)
	rl2_destroy_pixel (kill_no_data);
    return RL2_ERROR;
}

RL2_DECLARE rl2PalettePtr
rl2_get_dbms_palette (sqlite3 * handle, const char *coverage)
{
/* attempting to retrieve a Coverage's Palette from the DBMS */
    rl2PalettePtr palette = NULL;
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (handle == NULL || coverage == NULL)
	return NULL;

    sql = sqlite3_mprintf ("SELECT palette FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }

    if (palette == NULL)
	goto error;
    sqlite3_finalize (stmt);
    return palette;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

RL2_DECLARE int
rl2_update_dbms_palette (sqlite3 * handle, const char *coverage,
			 rl2PalettePtr palette)
{
/* attempting to update a Coverage's Palette into the DBMS */
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned short num_entries;
    unsigned char *blob;
    int blob_size;
    sqlite3_stmt *stmt = NULL;
    char *sql;
    int ret;
    if (handle == NULL || coverage == NULL || palette == NULL)
	return RL2_ERROR;

    sql =
	sqlite3_mprintf ("SELECT sample_type, pixel_type FROM raster_coverages "
			 "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *sample =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *pixel =
		    (const char *) sqlite3_column_text (stmt, 1);
		if (strcmp (sample, "1-BIT") == 0)
		    sample_type = RL2_SAMPLE_1_BIT;
		if (strcmp (sample, "2-BIT") == 0)
		    sample_type = RL2_SAMPLE_2_BIT;
		if (strcmp (sample, "4-BIT") == 0)
		    sample_type = RL2_SAMPLE_4_BIT;
		if (strcmp (sample, "UINT8") == 0)
		    sample_type = RL2_SAMPLE_UINT8;
		if (strcmp (pixel, "PALETTE") == 0)
		    pixel_type = RL2_PIXEL_PALETTE;
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* testing for self-consistency */
    if (pixel_type != RL2_PIXEL_PALETTE)
	goto error;
    if (rl2_get_palette_entries (palette, &num_entries) != RL2_OK)
	goto error;
    switch (sample_type)
      {
      case RL2_SAMPLE_UINT8:
	  if (num_entries > 256)
	      goto error;
	  break;
      case RL2_SAMPLE_1_BIT:
	  if (num_entries > 2)
	      goto error;
	  break;
      case RL2_SAMPLE_2_BIT:
	  if (num_entries > 4)
	      goto error;
	  break;
      case RL2_SAMPLE_4_BIT:
	  if (num_entries > 16)
	      goto error;
	  break;
      default:
	  goto error;
      };

    if (rl2_serialize_dbms_palette (palette, &blob, &blob_size) != RL2_OK)
	goto error;
    sql = sqlite3_mprintf ("UPDATE raster_coverages SET palette = ? "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  sqlite3_finalize (stmt);
	  return RL2_OK;
      }
    fprintf (stderr,
	     "sqlite3_step() error: UPDATE raster_coverages \"%s\"\n",
	     sqlite3_errmsg (handle));

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

static void
set_remapped_palette (rl2PrivTiffOriginPtr origin, rl2PalettePtr palette)
{
/* installing a remapped Palette into the TIFF origin */
    int j;
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;

    if (plt->nEntries != origin->remapMaxPalette)
      {
	  /* reallocating the remapped palette */
	  if (origin->remapRed != NULL)
	      free (origin->remapRed);
	  if (origin->remapGreen != NULL)
	      free (origin->remapGreen);
	  if (origin->remapBlue != NULL)
	      free (origin->remapBlue);
	  origin->remapMaxPalette = plt->nEntries;
	  origin->remapRed = malloc (origin->remapMaxPalette);
	  origin->remapGreen = malloc (origin->remapMaxPalette);
	  origin->remapBlue = malloc (origin->remapMaxPalette);
      }
    for (j = 0; j < plt->nEntries; j++)
      {
	  entry = plt->entries + j;
	  origin->remapRed[j] = entry->red;
	  origin->remapGreen[j] = entry->green;
	  origin->remapBlue[j] = entry->blue;
      }
}

RL2_DECLARE int
rl2_check_dbms_palette (sqlite3 * handle, rl2CoveragePtr coverage,
			rl2TiffOriginPtr tiff)
{
/*attempting to merge/update a Coverage's Palette */
    int i;
    int j;
    int changed = 0;
    int maxPalette = 0;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    int ok;
    rl2PalettePtr palette = NULL;
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr plt;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (cvg == NULL || origin == NULL)
	return RL2_ERROR;
    palette = rl2_get_dbms_palette (handle, cvg->coverageName);
    if (palette == NULL)
	goto error;
    plt = (rl2PrivPalettePtr) palette;
    for (j = 0; j < plt->nEntries; j++)
      {
	  entry = plt->entries + j;
	  ok = 0;
	  for (i = 0; i < maxPalette; i++)
	    {
		if (red[i] == entry->red && green[i] == entry->green
		    && blue[i] == entry->blue)
		  {
		      ok = 1;
		      break;
		  }
	    }
	  if (ok)
	      continue;
	  if (maxPalette == 256)
	      goto error;
	  red[maxPalette] = entry->red;
	  green[maxPalette] = entry->green;
	  blue[maxPalette] = entry->blue;
	  maxPalette++;
      }

    for (i = 0; i < origin->maxPalette; i++)
      {
	  /* checking TIFF palette entries */
	  unsigned char tiff_red = origin->red[i];
	  unsigned char tiff_green = origin->green[i];
	  unsigned char tiff_blue = origin->blue[i];
	  ok = 0;
	  for (j = 0; j < maxPalette; j++)
	    {
		if (tiff_red == red[j] && tiff_green == green[j]
		    && tiff_blue == blue[j])
		  {
		      /* found a matching color */
		      ok = 1;
		      break;
		  }
	    }
	  if (!ok)
	    {
		/* attempting to insert a new color into the pseudo-Palette */
		if (maxPalette == 256)
		    goto error;
		red[maxPalette] = tiff_red;
		green[maxPalette] = tiff_green;
		blue[maxPalette] = tiff_blue;
		maxPalette++;
		changed = 1;
	    }
      }
    if (changed)
      {
	  /* updating the DBMS Palette */
	  rl2PalettePtr plt2 = rl2_create_palette (maxPalette);
	  if (plt2 == NULL)
	      goto error;
	  rl2_destroy_palette (palette);
	  palette = plt2;
	  for (j = 0; j < maxPalette; j++)
	      rl2_set_palette_color (palette, j, red[j], green[j], blue[j]);
	  if (rl2_update_dbms_palette (handle, cvg->coverageName, palette) !=
	      RL2_OK)
	      goto error;
      }
    set_remapped_palette (origin, palette);
    rl2_destroy_palette (palette);
    return RL2_OK;

  error:
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_update_dbms_coverage (sqlite3 * handle, const char *coverage)
{
/*attempting to update a Coverage (statistics and extent) */
    int ret;
    char *sql;
    char *xtable;
    char *xxtable;
    rl2RasterStatisticsPtr coverage_stats = NULL;
    unsigned char *blob_stats;
    int blob_stats_sz;
    int first;
    sqlite3_stmt *stmt_ext_in = NULL;
    sqlite3_stmt *stmt_ext_out = NULL;
    sqlite3_stmt *stmt_stats_in = NULL;
    sqlite3_stmt *stmt_stats_out = NULL;

/* Extent query stmt */
    xtable = sqlite3_mprintf ("%s_sections", coverage);
    xxtable = gaiaDoubleQuotedSql (xtable);
    sqlite3_free (xtable);
    sql =
	sqlite3_mprintf
	("SELECT Min(MbrMinX(geometry)), Min(MbrMinY(geometry)), "
	 "Max(MbrMaxX(geometry)), Max(MbrMaxY(geometry)) " "FROM \"%s\"",
	 xxtable);
    free (xxtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT Coverage extent SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
/* Extent update stmt */
    sql = sqlite3_mprintf ("UPDATE raster_coverages SET extent_minx = ?, "
			   "extent_miny = ?, extent_maxx = ?, extent_maxy = ? "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE Coverage extent SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* querying the extent */
	  ret = sqlite3_step (stmt_ext_in);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		double minx = sqlite3_column_double (stmt_ext_in, 0);
		double miny = sqlite3_column_double (stmt_ext_in, 1);
		double maxx = sqlite3_column_double (stmt_ext_in, 2);
		double maxy = sqlite3_column_double (stmt_ext_in, 3);

		/* updating the extent */
		sqlite3_reset (stmt_ext_out);
		sqlite3_clear_bindings (stmt_ext_out);
		sqlite3_bind_double (stmt_ext_out, 1, minx);
		sqlite3_bind_double (stmt_ext_out, 2, miny);
		sqlite3_bind_double (stmt_ext_out, 3, maxx);
		sqlite3_bind_double (stmt_ext_out, 4, maxy);
		ret = sqlite3_step (stmt_ext_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    break;
		else
		  {
		      fprintf (stderr,
			       "UPDATE Coverage Extent sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT Coverage Extent sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_ext_in);
    sqlite3_finalize (stmt_ext_out);
    stmt_ext_in = NULL;
    stmt_ext_out = NULL;

/* Raster Statistics query stmt */
    xtable = sqlite3_mprintf ("%s_sections", coverage);
    xxtable = gaiaDoubleQuotedSql (xtable);
    sqlite3_free (xtable);
    sql = sqlite3_mprintf ("SELECT statistics FROM \"%s\"", xxtable);
    free (xxtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT Coverage Statistics SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
/* Raster Statistics update stmt */
    sql = sqlite3_mprintf ("UPDATE raster_coverages SET statistics = ? "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE Coverage Statistics SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    first = 1;
    while (1)
      {
	  /* querying the statistics */
	  ret = sqlite3_step (stmt_stats_in);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		rl2RasterStatisticsPtr stats;
		blob_stats =
		    (unsigned char *) sqlite3_column_blob (stmt_stats_in, 0);
		blob_stats_sz = sqlite3_column_bytes (stmt_stats_in, 0);
		stats =
		    rl2_deserialize_dbms_raster_statistics (blob_stats,
							    blob_stats_sz);
		if (stats == NULL)
		    goto error;

		if (first)
		  {
		      double no_data;
		      double count;
		      unsigned char sample_type;
		      unsigned char num_bands;
		      if (rl2_get_raster_statistics_summary
			  (stats, &no_data, &count, &sample_type,
			   &num_bands) != RL2_OK)
			  goto error;
		      coverage_stats =
			  rl2_create_raster_statistics (sample_type, num_bands);
		      if (coverage_stats == NULL)
			  goto error;
		      first = 0;
		  }

		rl2_aggregate_raster_statistics (stats, coverage_stats);
		rl2_destroy_raster_statistics (stats);
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT Coverage Statistics sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    if (coverage_stats == NULL)
	goto error;

    /* updating the statistics */
    sqlite3_reset (stmt_stats_out);
    sqlite3_clear_bindings (stmt_stats_out);
    rl2_serialize_dbms_raster_statistics (coverage_stats, &blob_stats,
					  &blob_stats_sz);
    sqlite3_bind_blob (stmt_stats_out, 1, blob_stats, blob_stats_sz, free);
    ret = sqlite3_step (stmt_stats_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "UPDATE Coverage Statistics sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_finalize (stmt_stats_in);
    sqlite3_finalize (stmt_stats_out);
    rl2_destroy_raster_statistics (coverage_stats);
    return RL2_OK;

  error:
    if (stmt_ext_in != NULL)
	sqlite3_finalize (stmt_ext_in);
    if (stmt_ext_out != NULL)
	sqlite3_finalize (stmt_ext_out);
    if (stmt_stats_in != NULL)
	sqlite3_finalize (stmt_stats_in);
    if (stmt_stats_out != NULL)
	sqlite3_finalize (stmt_stats_out);
    if (coverage_stats != NULL)
	rl2_destroy_raster_statistics (coverage_stats);
    return RL2_ERROR;
}
