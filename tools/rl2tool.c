/* 
/ rl2_tool
/
/ a generic tool supporting RasterLite2 DataSources
/
/ version 1.0, 2013 June 7
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ Copyright (C) 2013  Alessandro Furieri
/
/    This program is free software: you can redistribute it and/or modify
/    it under the terms of the GNU General Public License as published by
/    the Free Software Foundation, either version 3 of the License, or
/    (at your option) any later version.
/
/    This program is distributed in the hope that it will be useful,
/    but WITHOUT ANY WARRANTY; without even the implied warranty of
/    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/    GNU General Public License for more details.
/
/    You should have received a copy of the GNU General Public License
/    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <rasterlite2/rasterlite2.h>
#include <rasterlite2/rl2tiff.h>

#include <spatialite/gaiaaux.h>
#include <spatialite.h>

#define ARG_NONE		0
#define ARG_MODE_CREATE		1
#define ARG_MODE_DROP		2
#define ARG_MODE_IMPORT		3
#define ARG_MODE_DELETE		4
#define ARG_MODE_PYRAMIDIZE	5
#define ARG_MODE_LIST		6
#define ARG_MODE_CATALOG	7
#define ARG_MODE_CHECK		8

#define ARG_DB_PATH		10
#define ARG_SRC_PATH		11
#define ARG_WF_PATH		12
#define ARG_COVERAGE		13
#define ARG_SECTION		14
#define ARG_SAMPLE		15
#define ARG_PIXEL		16
#define ARG_NUM_BANDS		17
#define ARG_COMPRESSION		18
#define ARG_QUALITY		19
#define ARG_TILE_WIDTH		20
#define ARG_TILE_HEIGHT		21
#define ARG_SRID		22
#define ARG_RESOLUTION		23
#define ARG_X_RESOLUTION	24
#define ARG_Y_RESOLUTION	25

#define ARG_CACHE_SIZE		99

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

struct pyramid_params
{
/* a struct used to pass Pyramidization params */
    const char *coverage;
    sqlite3_int64 section_id;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char bands;
    unsigned char compression;
    int quality;
    int tile_width;
    int tile_height;
    int srid;
    double x_res;
    double y_res;
    const char *name;
    int width;
    int height;
    double min_x;
    double min_y;
    double max_x;
    double max_y;
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    unsigned char *bufpix;
    int bufpix_size;
    unsigned char *mask;
    int mask_size;
    sqlite3_stmt *query_stmt;
    sqlite3_stmt *tiles_stmt;
    sqlite3_stmt *data_stmt;
};

static char *
get_section_name (const char *src_path)
{
/* attempting to extract the section name from source path */
    int pos1 = 0;
    int pos2 = 0;
    int len;
    char *name;
    const char *p;
    if (src_path == NULL)
	return NULL;
    pos2 = strlen (src_path) - 1;
    pos1 = 0;
    p = src_path + pos2;
    while (p >= src_path)
      {
	  if (*p == '.')
	      pos2 = (p - 1) - src_path;
	  if (*p == '/')
	    {
		pos1 = (p + 1) - src_path;
		break;
	    }
	  p--;
      }
    len = pos2 - pos1 + 1;
    name = malloc (len + 1);
    memset (name, '\0', len + 1);
    memcpy (name, src_path + pos1, len);
    return name;
}

static gaiaGeomCollPtr
build_extent (int srid, double minx, double miny, double maxx, double maxy)
{
/* building an MBR (Envelope) */
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
    geom->Srid = srid;
    pg = gaiaAddPolygonToGeomColl (geom, 5, 0);
    rng = pg->Exterior;
    gaiaSetPoint (rng->Coords, 0, minx, miny);
    gaiaSetPoint (rng->Coords, 1, maxx, miny);
    gaiaSetPoint (rng->Coords, 2, maxx, maxy);
    gaiaSetPoint (rng->Coords, 3, minx, maxy);
    gaiaSetPoint (rng->Coords, 4, minx, miny);
    return geom;
}

static int
set_connection (sqlite3 * handle, int journal_off)
{
/* properly setting up the connection */
    int ret;
    char *sql_err = NULL;

    if (journal_off)
      {
	  /* disabling the journal: unsafe but faster */
	  ret =
	      sqlite3_exec (handle, "PRAGMA journal_mode = OFF",
			    NULL, NULL, &sql_err);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "PRAGMA journal_mode=OFF error: %s\n",
			 sql_err);
		sqlite3_free (sql_err);
		return 0;
	    }
      }

/* enabling foreign key constraints */
    ret =
	sqlite3_exec (handle, "PRAGMA foreign_keys = 1", NULL, NULL, &sql_err);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "PRAGMA foreign_keys = 1 error: %s\n", sql_err);
	  sqlite3_free (sql_err);
	  return 0;
      }

    return 1;
}

static int
exec_create (sqlite3 * handle, const char *coverage,
	     int sample, int pixel, int num_bands, int compression, int quality,
	     int tile_width, int tile_height, int srid, double x_res,
	     double y_res)
{
/* performing CREATE */
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
    sqlite3_stmt *stmt;
    const char *xsample = "UNKNOWN";
    const char *xpixel = "UNKNOWN";
    const char *xcompression = "UNKNOWN";

/* inserting into "raster_coverages" */
    sql = "INSERT INTO raster_coverages (coverage_name, sample_type, "
	"pixel_type, num_bands, compression, quality, tile_width, "
	"tile_height, horz_resolution, vert_resolution, srid, "
	"nodata_pixel) VALUES (Lower(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NULL)";
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
	  num_bands = 1;
	  break;
      case RL2_PIXEL_PALETTE:
	  xpixel = "PALETTE";
	  num_bands = 1;
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  xpixel = "GRAYSCALE";
	  num_bands = 1;
	  break;
      case RL2_PIXEL_RGB:
	  xpixel = "RGB";
	  num_bands = 3;
	  break;
      case RL2_PIXEL_MULTIBAND:
	  xpixel = "MULTIBAND";
	  break;
      case RL2_PIXEL_DATAGRID:
	  xpixel = "DATAGRID";
	  num_bands = 1;
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
      case RL2_COMPRESSION_GIF:
	  xcompression = "GIF";
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

/* creating the LEVELS table */
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
	  fprintf (stderr, "CREATE TABLE \"%s\" error: %s\n", xxcoverage,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* creating the SECTIONS table */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\tsection_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tsection_name TEXT NOT NULL,\n"
			   "\twidth INTEGER NOT NULL,\n"
			   "\theight INTEGER NOT NULL,\n"
			   "\tfile_path TEXT)", xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s\" error: %s\n", xxcoverage,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* creating the SECTIONS geometry */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("SELECT AddGeometryColumn("
			   "%Q, 'geometry', %d, 'POLYGON', 'XY')", xcoverage,
			   srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn \"%s\" error: %s\n", xcoverage,
		   sql_err);
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
	  fprintf (stderr, "CreateSpatialIndex \"%s\" error: %s\n", xcoverage,
		   sql_err);
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

/* creating the TILES table */
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
	  fprintf (stderr, "CREATE TABLE \"%s\" error: %s\n", xxcoverage,
		   sql_err);
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
	  fprintf (stderr, "AddGeometryColumn \"%s\" error: %s\n", xcoverage,
		   sql_err);
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
	  fprintf (stderr, "CreateSpatialIndex \"%s\" error: %s\n", xcoverage,
		   sql_err);
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
	  fprintf (stderr, "CREATE TABLE \"%s\" error: %s\n", xxcoverage,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

    fprintf (stderr, "\rRaster Coverage \"%s\" succesfully created\n",
	     coverage);
    return 1;
}

static rl2CoveragePtr
create_coverage_obj (sqlite3 * handle, const char *coverage)
{
/* attempting to create a Coverage Object */
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

static int
exec_import (sqlite3 * handle, const char *src_path,
	     const char *coverage, const char *section, const char *wf_path,
	     int pyramidize)
{
/* performing IMPORT */
    int ret;
    char *sql;
    rl2TiffOriginPtr origin = NULL;
    rl2CoveragePtr cvg = NULL;
    rl2RasterPtr raster = NULL;
    int row;
    int col;
    int tile_w;
    int tile_h;
    unsigned short width;
    unsigned short height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    unsigned char compression;
    int quality;
    int srid;
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    char *table;
    char *xtable;
    sqlite3_int64 section_id;
    sqlite3_int64 tile_id;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_levl = NULL;

    cvg = create_coverage_obj (handle, coverage);
    if (cvg == NULL)
	goto error;

    tile_w = rl2_get_coverage_tile_width (cvg);
    tile_h = rl2_get_coverage_tile_height (cvg);
    fprintf (stderr, "tile_w=%d, tile_h=%d\n", tile_w, tile_h);
    compression = rl2_get_coverage_compression (cvg);
    quality = rl2_get_coverage_quality (cvg);

    origin = rl2_create_tiff_origin (src_path, RL2_TIFF_GEOTIFF);
    if (origin == NULL)
	goto error;

    fprintf (stderr, "TIFF path=%s\n", rl2_get_tiff_origin_path (origin));
    ret = rl2_get_tiff_origin_size (origin, &width, &height);
    if (ret == RL2_OK)
	fprintf (stderr, "width=%d height=%d\n", width, height);
    ret = rl2_get_tiff_origin_srid (origin, &srid);
    if (ret == RL2_OK)
	fprintf (stderr, "SRID=%d\n", srid);
    ret = rl2_get_tiff_origin_extent (origin, &minx, &miny, &maxx, &maxy);
    if (ret == RL2_OK)
	fprintf (stderr, "%1.2f %1.2f %1.2f %1.2f\n", minx, miny, maxx, maxy);
    ret = rl2_get_tiff_origin_resolution (origin, &res_x, &res_y);
    if (ret == RL2_OK)
	fprintf (stderr, "Hres=%1.6f Vres=%1.6f\n", res_x, res_y);

    if (rl2_eval_tiff_origin_compatibility (cvg, origin) != RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch\n");
	  goto error;
      }

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (section_id, section_name, file_path, "
	 "width, height, geometry) VALUES (NULL, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO sections SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_levels", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	 "x_resolution_1_1, y_resolution_1_1, "
	 "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	 "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	 "VALUES (0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO levels SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 0, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tils, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, tile_data_odd, tile_data_even) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tile_data SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* INSERTing the section */
    sqlite3_reset (stmt_sect);
    sqlite3_clear_bindings (stmt_sect);
    if (section != NULL)
	sqlite3_bind_text (stmt_sect, 1, section, strlen (section),
			   SQLITE_STATIC);
    else
      {
	  char *sect_name = get_section_name (src_path);
	  if (sect_name == NULL)
	      sqlite3_bind_null (stmt_sect, 1);
	  else
	      sqlite3_bind_text (stmt_sect, 1, sect_name, strlen (sect_name),
				 free);
      }
    sqlite3_bind_text (stmt_sect, 2, src_path, strlen (src_path),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt_sect, 3, width);
    sqlite3_bind_int (stmt_sect, 4, height);
    geom = build_extent (srid, minx, miny, maxx, maxy);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    gaiaFreeGeomColl (geom);
    sqlite3_bind_blob (stmt_sect, 5, blob, blob_size, free);
    ret = sqlite3_step (stmt_sect);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	section_id = sqlite3_last_insert_rowid (handle);
    else
      {
	  fprintf (stderr,
		   "INSERT INTO sections; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_finalize (stmt_sect);
    stmt_sect = NULL;

/* INSERTing the base-levels */
    sqlite3_reset (stmt_levl);
    sqlite3_clear_bindings (stmt_levl);
    sqlite3_bind_double (stmt_levl, 1, res_x);
    sqlite3_bind_double (stmt_levl, 2, res_y);
    sqlite3_bind_double (stmt_levl, 3, res_x * 2.0);
    sqlite3_bind_double (stmt_levl, 4, res_y * 2.0);
    sqlite3_bind_double (stmt_levl, 5, res_x * 4.0);
    sqlite3_bind_double (stmt_levl, 6, res_y * 4.0);
    sqlite3_bind_double (stmt_levl, 7, res_x * 8.0);
    sqlite3_bind_double (stmt_levl, 8, res_y * 8.0);
    ret = sqlite3_step (stmt_levl);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO levels; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_finalize (stmt_levl);
    stmt_levl = NULL;

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster = rl2_get_tile_from_tiff_origin (cvg, origin, row, col);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "ERROR: unable to get a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }
		if (rl2_raster_encode
		    (raster, compression, &blob_odd, &blob_odd_sz, &blob_even,
		     &blob_even_sz, quality, 1) != RL2_OK)
		  {
		      fprintf (stderr,
			       "ERROR: unable to encode a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }
		rl2_destroy_raster (raster);
		raster = NULL;
		/* INSERTing the tile */
		sqlite3_reset (stmt_tils);
		sqlite3_clear_bindings (stmt_tils);
		sqlite3_bind_int64 (stmt_tils, 1, section_id);
		tile_maxx = tile_minx + ((double) tile_w * res_x);
		tile_miny = tile_maxy - ((double) tile_h * res_y);
		geom =
		    build_extent (srid, tile_minx, tile_miny, tile_maxx,
				  tile_maxy);
		gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
		gaiaFreeGeomColl (geom);
		sqlite3_bind_blob (stmt_tils, 2, blob, blob_size, free);
		ret = sqlite3_step (stmt_tils);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      fprintf (stderr,
			       "INSERT INTO tiles; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
		tile_id = sqlite3_last_insert_rowid (handle);
		/* INSERTing tile data */
		sqlite3_reset (stmt_data);
		sqlite3_clear_bindings (stmt_data);
		tile_id = sqlite3_bind_int64 (stmt_data, 1, tile_id);
		sqlite3_bind_blob (stmt_data, 2, blob_odd, blob_odd_sz, free);
		if (blob_even == NULL)
		    sqlite3_bind_null (stmt_data, 3);
		else
		    sqlite3_bind_blob (stmt_data, 3, blob_even, blob_even_sz,
				       free);
		ret = sqlite3_step (stmt_data);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      fprintf (stderr,
			       "INSERT INTO tile_data; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
		blob_odd = NULL;
		blob_even = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);

    rl2_destroy_tiff_origin (origin);
    rl2_destroy_coverage (cvg);
    return 1;

  error:
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    if (stmt_sect != NULL)
	sqlite3_finalize (stmt_sect);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (origin != NULL)
	rl2_destroy_tiff_origin (origin);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return 0;
}

static int
exec_drop (sqlite3 * handle, const char *coverage)
{
/* performing DROP */
    int ret;
    char *sql;
    char *sql_err = NULL;
    rl2CoveragePtr cvg = NULL;
    char *table;
    char *xtable;

    cvg = create_coverage_obj (handle, coverage);
    if (cvg == NULL)
	goto error;

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
    fprintf (stderr, "%s\n", sql);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE raster_coverage \"%s\" error: %s\n",
		   coverage, sql_err);
	  sqlite3_free (sql_err);
	  goto error;
      }

    rl2_destroy_coverage (cvg);
    return 1;

  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return 0;
}

static int
check_raster_section (sqlite3 * handle, const char *coverage,
		      const char *section, sqlite3_int64 * section_id)
{
/* testing if the named Raster Section does exist */
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
	return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
exec_delete (sqlite3 * handle, const char *coverage, const char *section)
{
/* deleting a Raster Section */
    int ret;
    char *sql;
    rl2CoveragePtr cvg = NULL;
    sqlite3_int64 section_id;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt = NULL;

    cvg = create_coverage_obj (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (!check_raster_section (handle, coverage, section, &section_id))
      {
	  fprintf (stderr,
		   "Section \"%s\" does not exists in Coverage \"%s\"\n",
		   section, coverage);
	  goto error;
      }

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
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return 0;
}

static void
do_transparent_mask (struct pyramid_params *params, int col, int row)
{
/* marking transparent pixels into the mask */
    int r;
    int base_r = row * params->tile_height;
    unsigned char *p = params->mask;

    for (r = base_r; r < base_r + params->tile_height; r += 16)
      {
	  int c;
	  int base_c = col * params->tile_width;
	  for (c = base_c; c < base_c + params->tile_width; c += 16)
	      *p++ = 0;
      }
}

static void
rescale_pixels (struct pyramid_params *params, int row, int col,
		rl2RasterPtr rst)
{
/* rescaling pixels */
    int r;
    unsigned char *ppxl = params->bufpix;
    unsigned char *pmsk = params->mask;
    int pix_sz = 1;

/* computing the sample size */
    switch (params->sample_type)
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

    for (r = 0; r < params->tile_height; r += 16)
      {
	  int c;
	  for (c = 0; c < params->tile_width; c += 16)
	    {
		rl2PixelPtr pixel = rl2_rescale_block (rst, r, c, 16);
		if (pixel != NULL)
		  {
		      int band;
		      for (band = 0; band < params->bands; band++)
			{
			    void *pxl;
			    char v8;
			    unsigned char vU8;
			    short v16;
			    unsigned short vU16;
			    int v32;
			    unsigned int vU32;
			    float vFloat;
			    double vDouble;
			    switch (params->sample_type)
			      {
			      case RL2_SAMPLE_INT8:
				  rl2_get_pixel_sample_int8 (pixel, &v8);
				  pxl = &v8;
				  break;
			      case RL2_SAMPLE_UINT8:
				  rl2_get_pixel_sample_uint8 (pixel, band,
							      &vU8);
				  pxl = &vU8;
				  break;
			      case RL2_SAMPLE_INT16:
				  rl2_get_pixel_sample_int16 (pixel, &v16);
				  pxl = &v16;
				  break;
			      case RL2_SAMPLE_UINT16:
				  rl2_get_pixel_sample_uint16 (pixel, band,
							       &vU16);
				  pxl = &vU16;
				  break;
			      case RL2_SAMPLE_INT32:
				  rl2_get_pixel_sample_int32 (pixel, &v32);
				  pxl = &v32;
				  break;
			      case RL2_SAMPLE_UINT32:
				  rl2_get_pixel_sample_uint32 (pixel, &vU32);
				  pxl = &vU32;
				  break;
			      case RL2_SAMPLE_FLOAT:
				  rl2_get_pixel_sample_float (pixel, &vFloat);
				  pxl = &vFloat;
				  break;
			      case RL2_SAMPLE_DOUBLE:
				  rl2_get_pixel_sample_double (pixel, &vDouble);
				  pxl = &vDouble;
				  break;
			      };
			    memcpy (ppxl, pxl, pix_sz);
			    *ppxl += pix_sz;
			}
		      rl2_destroy_pixel (pixel);
		      *pmsk++ = 1;
		  }
		else
		    *pmsk++ = 0;
	    }
      }
}

static int
build_pyramid_tile (sqlite3 * handle, struct pyramid_params *params)
{
/* building a single Pyramid tile */
    int row;
    int col;
    int ret;
    rl2RasterPtr raster;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    sqlite3_int64 tile_id;
    sqlite3_stmt *stmt = params->query_stmt;
    sqlite3_stmt *stmt_tils = params->tiles_stmt;
    sqlite3_stmt *stmt_data = params->data_stmt;
    double cy =
	params->tile_maxy - ((params->tile_height * params->y_res) / 2.0);

    for (row = 0; row < 16; row++)
      {
	  double cx =
	      params->tile_minx + ((params->tile_width * params->x_res) / 2.0);
	  for (col = 0; col < 16; col++)
	    {
		/* retrieving lower-level tiles */
		int done = 0;
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_int64 (stmt, 1, params->section_id);
		sqlite3_bind_double (stmt, 2, cx);
		sqlite3_bind_double (stmt, 3, cy);
		sqlite3_bind_double (stmt, 4, cx);
		sqlite3_bind_double (stmt, 5, cy);
		while (1)
		  {
		      /* scrolling the result set rows */
		      int ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE)
			  break;	/* end of result set */
		      if (ret == SQLITE_ROW)
			{
			    rl2RasterPtr rst;
			    const unsigned char *blob_odd = NULL;
			    int blob_odd_sz = 0;
			    const unsigned char *blob_even = NULL;
			    int blob_even_sz = 0;
			    sqlite3_int64 tile_id =
				sqlite3_column_int64 (stmt, 0);
			    if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
			      {
				  blob_odd = sqlite3_column_blob (stmt, 1);
				  blob_odd_sz = sqlite3_column_bytes (stmt, 1);
			      }
			    if (sqlite3_column_type (stmt, 2) == SQLITE_BLOB)
			      {
				  blob_even = sqlite3_column_blob (stmt, 2);
				  blob_even_sz = sqlite3_column_bytes (stmt, 2);
			      }
			    rst =
				rl2_raster_decode (RL2_SCALE_1, blob_odd,
						   blob_odd_sz, blob_even,
						   blob_even_sz);
			    if (rst == NULL)
			      {
				  fprintf (stderr,
					   "ERROR: unable to decode Tile ID=%lld\n",
					   tile_id);
				  return 0;
			      }
			    rescale_pixels (params, row, col, rst);
			    done = 1;
			}
		      else
			{
			    fprintf (stderr, "SQL error: %s\n%s\n",
				     sqlite3_errmsg (handle));
			    return 0;
			}
		  }
		cx += params->tile_width * params->x_res;
		if (!done)
		  {
		      /* lower-level tile not found - updating the Mask */
		      do_transparent_mask (params, col, row);
		  }
	    }
	  cy -= params->tile_height * params->y_res;
      }

/* saving the current Pyramid Tile */
    raster =
	rl2_create_raster (params->tile_width, params->tile_height,
			   params->sample_type, params->pixel_type,
			   params->bands, params->bufpix, params->bufpix_size,
			   NULL, params->mask, params->mask_size, NULL);
    if (rl2_raster_encode
	(raster, params->compression, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, params->quality, 1) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to encode a tile [Row=%d Col=%d]\n",
		   row, col);
	  return 0;
      }
    rl2_destroy_raster (raster);
    raster = NULL;
/* INSERTing the tile */
    sqlite3_reset (stmt_tils);
    sqlite3_clear_bindings (stmt_tils);
    sqlite3_bind_int64 (stmt_tils, 1, params->section_id);
    /*
       tile_maxx = tile_minx + ((double) tile_w * res_x);
       tile_miny = tile_maxy - ((double) tile_h * res_y);
     */
    geom =
	build_extent (params->srid, params->tile_minx, params->tile_miny,
		      params->tile_maxx, params->tile_maxy);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    gaiaFreeGeomColl (geom);
    sqlite3_bind_blob (stmt_tils, 2, blob, blob_size, free);
    ret = sqlite3_step (stmt_tils);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tiles; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  return 0;
      }
    tile_id = sqlite3_last_insert_rowid (handle);
/* INSERTing tile data */
    sqlite3_reset (stmt_data);
    sqlite3_clear_bindings (stmt_data);
    tile_id = sqlite3_bind_int64 (stmt_data, 1, tile_id);
    sqlite3_bind_blob (stmt_data, 2, blob_odd, blob_odd_sz, free);
    if (blob_even == NULL)
	sqlite3_bind_null (stmt_data, 3);
    else
	sqlite3_bind_blob (stmt_data, 3, blob_even, blob_even_sz, free);
    ret = sqlite3_step (stmt_data);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tile_data; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  return 0;
      }
    blob_odd = NULL;
    blob_even = NULL;

    return 1;
}

static int
build_pyramids (sqlite3 * handle, struct pyramid_params *params)
{
/* building Pyramid Levels for a single Section */
    int row;
    int col;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    char *table_tiles;
    char *xtable_tiles;
    char *table_data;
    char *xtable_data;
    sqlite3_stmt *stmt = NULL;

    params->tile_maxy = params->max_y;
    fprintf (stderr, "Building Pyramid Levels: Section \"%s\"\n", params->name);

/* attempting to insert a new Pyramid Level definition */
    table = sqlite3_mprintf ("%s_levels", params->coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	 "x_resolution_1_1, y_resolution_1_1, "
	 "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	 "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	 "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO levels SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  return 0;
      }
/* INSERTing Pyramid-level #1 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, params->x_res * 16);
    sqlite3_bind_double (stmt, 2, params->y_res * 16);
    sqlite3_bind_double (stmt, 3, params->x_res * 32);
    sqlite3_bind_double (stmt, 4, params->y_res * 32);
    sqlite3_bind_double (stmt, 5, params->x_res * 64);
    sqlite3_bind_double (stmt, 6, params->y_res * 64);
    sqlite3_bind_double (stmt, 7, params->x_res * 128);
    sqlite3_bind_double (stmt, 8, params->y_res * 128);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO levels; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

    table_tiles = sqlite3_mprintf ("%s_tiles", params->coverage);
    xtable_tiles = gaiaDoubleQuotedSql (table_tiles);
    table_data = sqlite3_mprintf ("%s_tile_data", params->coverage);
    xtable_data = gaiaDoubleQuotedSql (table_data);
    sqlite3_free (table_data);
    sql =
	sqlite3_mprintf ("SELECT d.tile_id, d.tile_data_odd, d.tile_data_even "
			 "FROM \"%s\" AS t JOIN \"%s\" AS d ON (t.tile_id = d.tile_id) "
			 "WHERE t.section_id = ? AND t.pyramid_level = 0 "
			 "AND ST_Intersects(t.geometry, MakePoint(?, ?)) = 1 "
			 "AND t.ROWID IN (SELECT ROWID FROM SpatialIndex "
			 "WHERE f_table_name = %Q AND search_frame = MakePoint(?, ?))",
			 xtable_tiles, xtable_data, table_tiles);
    sqlite3_free (table_tiles);
    free (xtable_tiles);
    free (xtable_data);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    params->query_stmt = stmt;

    table = sqlite3_mprintf ("%s_tiles", params->coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 1, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }
    params->tiles_stmt = stmt;

    table = sqlite3_mprintf ("%s_tile_data", params->coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, tile_data_odd, tile_data_even) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tile_data SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
    params->data_stmt = stmt;

    for (row = 0; row < params->height; row += params->tile_height * 16)
      {
	  params->tile_minx = params->min_x;
	  params->tile_miny =
	      params->tile_maxy - ((params->tile_height * 16) * params->y_res);
	  params->tile_miny =
	      params->tile_maxy - ((params->tile_height * 16) * params->y_res);
	  for (col = 0; col < params->width; col += params->tile_width * 16)
	    {
		params->tile_maxx =
		    params->tile_minx +
		    ((params->tile_width * 16) * params->x_res);
		if (!build_pyramid_tile (handle, params))
		    goto error;
		params->tile_minx += (params->tile_width * 16) * params->x_res;
	    }
	  params->tile_maxy -= (params->tile_height * 16) * params->y_res;
      }
    sqlite3_finalize (params->query_stmt);
    params->query_stmt = NULL;
    return 1;

  error:
    if (params->query_stmt != NULL)
	sqlite3_finalize (params->query_stmt);
    params->query_stmt = NULL;
    return 0;
}

static int
exec_pyramidize (sqlite3 * handle, const char *coverage, const char *section,
		 int force_pyramid)
{
/* building Pyramid levels */
    int ret;
    char *sql;
    rl2CoveragePtr cvg = NULL;
    struct pyramid_params params;
    int pix_sz = 1;
    char *table_levels;
    char *xtable_levels;
    char *table_sections;
    char *xtable_sections;
    sqlite3_stmt *stmt = NULL;

    cvg = create_coverage_obj (handle, coverage);
    if (cvg == NULL)
	goto error;

    params.coverage = coverage;
    params.sample_type = rl2_get_coverage_sample_type (cvg);
    params.pixel_type = rl2_get_coverage_pixel_type (cvg);
    params.bands = rl2_get_coverage_bands (cvg);
    params.compression = rl2_get_coverage_compression (cvg);
    params.quality = rl2_get_coverage_quality (cvg);
    params.tile_width = rl2_get_coverage_tile_width (cvg);
    params.tile_height = rl2_get_coverage_tile_height (cvg);
    params.srid = rl2_get_coverage_srid (cvg);
    params.bufpix = NULL;
    params.mask = NULL;
    params.query_stmt = NULL;
    params.tiles_stmt = NULL;
    params.data_stmt = NULL;

/* allocating the pixels buffer and transparency mask */
    switch (params.sample_type)
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
    params.bufpix_size =
	params.tile_width * params.tile_height * pix_sz * params.bands;
    params.bufpix = malloc (params.bufpix_size);
    if (params.bufpix == NULL)
      {
	  fprintf (stderr, "Pyramidize: Insufficient Memory [pixels]\n");
	  goto error;
      }
    params.mask_size = params.tile_width * params.tile_height;
    params.mask = malloc (params.mask_size);
    if (params.mask == NULL)
      {
	  fprintf (stderr, "Pyramidize: Insufficient Memory [mask]\n");
	  goto error;
      }
    rl2_prime_void_tile (params.bufpix, params.tile_width, params.tile_height,
			 params.sample_type, params.bands);

    if (section != NULL)
      {
	  /* only a single selected Section */
	  if (!check_raster_section
	      (handle, coverage, section, &(params.section_id)))
	    {
		fprintf (stderr,
			 "Section \"%s\" does not exists in Coverage \"%s\"\n",
			 section, coverage);
		goto error;
	    }
	  table_sections = sqlite3_mprintf ("%s_sections", coverage);
	  xtable_sections = gaiaDoubleQuotedSql (table_sections);
	  sqlite3_free (table_sections);
	  table_levels = sqlite3_mprintf ("%s_levels", coverage);
	  xtable_levels = gaiaDoubleQuotedSql (table_levels);
	  sqlite3_free (table_levels);
	  sql =
	      sqlite3_mprintf ("SELECT l.x_resolution_1_1, l.y_resolution_1_1, "
			       "s.section_id, s.section_name, s.width, s.height, MbrMinX(s.geometry), "
			       "MbrMinY(s.geometry), MbrMaxX(s.geometry), MbrMaxY(s.geometry) "
			       "FROM \"%s\" AS l, \"%s\" AS s WHERE l.pyramid_level = 0 "
			       "AND s.section_id = %d", xtable_levels,
			       xtable_sections);
	  free (xtable_sections);
	  free (xtable_levels);
      }
    else
      {
	  /* all Sections */
	  table_sections = sqlite3_mprintf ("%s_sections", coverage);
	  xtable_sections = gaiaDoubleQuotedSql (table_sections);
	  sqlite3_free (table_sections);
	  table_levels = sqlite3_mprintf ("%s_levels", coverage);
	  xtable_levels = gaiaDoubleQuotedSql (table_levels);
	  sqlite3_free (table_levels);
	  sql =
	      sqlite3_mprintf ("SELECT l.x_resolution_1_1, l.y_resolution_1_1, "
			       "s.section_id, s.section_name, s.width, s.height, MbrMinX(s.geometry), "
			       "MbrMinY(s.geometry), MbrMaxX(s.geometry), MbrMaxY(s.geometry) "
			       "FROM \"%s\" AS l, \"%s\" AS s WHERE l.pyramid_level = 0",
			       xtable_levels, xtable_sections);
	  free (xtable_sections);
	  free (xtable_levels);
      }

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
		params.x_res = sqlite3_column_double (stmt, 0);
		params.y_res = sqlite3_column_double (stmt, 1);
		params.section_id = sqlite3_column_int64 (stmt, 2);
		params.name = (const char *) sqlite3_column_text (stmt, 3);
		params.width = sqlite3_column_int (stmt, 4);
		params.height = sqlite3_column_int (stmt, 5);
		params.min_x = sqlite3_column_double (stmt, 6);
		params.min_y = sqlite3_column_double (stmt, 7);
		params.max_x = sqlite3_column_double (stmt, 8);
		params.max_y = sqlite3_column_double (stmt, 9);
		if (!build_pyramids (handle, &params))
		  {
		      /* some unexpected error occurred */
		      sqlite3_finalize (stmt);
		      goto error;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    rl2_destroy_coverage (cvg);
    free (params.bufpix);
    free (params.mask);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (params.query_stmt != NULL)
	sqlite3_finalize (params.query_stmt);
    if (params.tiles_stmt != NULL)
	sqlite3_finalize (params.tiles_stmt);
    if (params.data_stmt != NULL)
	sqlite3_finalize (params.data_stmt);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    if (params.bufpix)
	free (params.bufpix);
    if (params.mask)
	free (params.mask);
    return 0;
}

static void
spatialite_autocreate (sqlite3 * db)
{
/* attempting to perform self-initialization for a newly created DB */
    int ret;
    char sql[1024];
    char *err_msg = NULL;
    int count;
    int i;
    char **results;
    int rows;
    int columns;

/* checking if this DB is really empty */
    strcpy (sql, "SELECT Count(*) from sqlite_master");
    ret = sqlite3_get_table (db, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	      count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (count > 0)
	return;

/* all right, it's empty: proceding to initialize */
    strcpy (sql, "SELECT InitSpatialMetadata(1)");
    ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
    strcpy (sql, "SELECT CreateRasterCoveragesTable()");
    ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
}

static void
open_db (const char *path, sqlite3 ** handle, int cache_size, void *cache)
{
/* opening the DB */
    sqlite3 *db_handle;
    int ret;
    char sql[1024];
    int spatialite_rs = 0;
    int spatialite_gc = 0;
    int rs_srid = 0;
    int auth_name = 0;
    int auth_srid = 0;
    int ref_sys_name = 0;
    int proj4text = 0;
    int f_table_name = 0;
    int f_geometry_column = 0;
    int coord_dimension = 0;
    int gc_srid = 0;
    int type = 0;
    int geometry_type = 0;
    int spatial_index_enabled = 0;
    const char *name;
    int i;
    char **results;
    int rows;
    int columns;

    *handle = NULL;
    printf ("SQLite version: %s\n", sqlite3_libversion ());
    printf ("SpatiaLite version: %s\n\n", spatialite_version ());

    ret =
	sqlite3_open_v2 (path, &db_handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open '%s': %s\n", path,
		   sqlite3_errmsg (db_handle));
	  sqlite3_close (db_handle);
	  return;
      }
    spatialite_init_ex (db_handle, cache, 0);
    spatialite_autocreate (db_handle);
    if (cache_size > 0)
      {
	  /* setting the CACHE-SIZE */
	  sprintf (sql, "PRAGMA cache_size=%d", cache_size);
	  sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
      }

/* checking the GEOMETRY_COLUMNS table */
    strcpy (sql, "PRAGMA table_info(geometry_columns)");
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, "f_table_name") == 0)
		    f_table_name = 1;
		if (strcasecmp (name, "f_geometry_column") == 0)
		    f_geometry_column = 1;
		if (strcasecmp (name, "coord_dimension") == 0)
		    coord_dimension = 1;
		if (strcasecmp (name, "srid") == 0)
		    gc_srid = 1;
		if (strcasecmp (name, "type") == 0)
		    type = 1;
		if (strcasecmp (name, "geometry_type") == 0)
		    geometry_type = 1;
		if (strcasecmp (name, "spatial_index_enabled") == 0)
		    spatial_index_enabled = 1;
	    }
      }
    sqlite3_free_table (results);
    if (f_table_name && f_geometry_column && type && coord_dimension
	&& gc_srid && spatial_index_enabled)
	spatialite_gc = 1;
    if (f_table_name && f_geometry_column && geometry_type && coord_dimension
	&& gc_srid && spatial_index_enabled)
	spatialite_gc = 3;

/* checking the SPATIAL_REF_SYS table */
    strcpy (sql, "PRAGMA table_info(spatial_ref_sys)");
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, "srid") == 0)
		    rs_srid = 1;
		if (strcasecmp (name, "auth_name") == 0)
		    auth_name = 1;
		if (strcasecmp (name, "auth_srid") == 0)
		    auth_srid = 1;
		if (strcasecmp (name, "ref_sys_name") == 0)
		    ref_sys_name = 1;
		if (strcasecmp (name, "proj4text") == 0)
		    proj4text = 1;
	    }
      }
    sqlite3_free_table (results);
    if (rs_srid && auth_name && auth_srid && ref_sys_name && proj4text)
	spatialite_rs = 1;
/* verifying the MetaData format */
    if (spatialite_gc && spatialite_rs)
	;
    else
	goto unknown;
    *handle = db_handle;
    return;

  unknown:
    if (db_handle)
	sqlite3_close (db_handle);
    fprintf (stderr, "DB '%s'\n", path);
    fprintf (stderr, "doesn't seems to contain valid Spatial Metadata ...\n\n");
    fprintf (stderr, "Please, initialize Spatial Metadata\n\n");
    return;
}

static int
check_create_args (const char *db_path, const char *coverage, int sample,
		   int pixel, int num_bands, int compression, int quality,
		   int tile_width, int tile_height, int srid, double x_res,
		   double y_res)
{
/* checking/printing CREATE args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool: request is CREATE\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    switch (sample)
      {
      case RL2_SAMPLE_1_BIT:
	  fprintf (stderr, "Sample Type: 1-BIT\n");
	  break;
      case RL2_SAMPLE_2_BIT:
	  fprintf (stderr, "Sample Type: 2-BIT\n");
	  break;
      case RL2_SAMPLE_4_BIT:
	  fprintf (stderr, "Sample Type: 4-BIT\n");
	  break;
      case RL2_SAMPLE_INT8:
	  fprintf (stderr, "Sample Type: INT8\n");
	  break;
      case RL2_SAMPLE_UINT8:
	  fprintf (stderr, "Sample Type: UINT8\n");
	  break;
      case RL2_SAMPLE_INT16:
	  fprintf (stderr, "Sample Type: INT16\n");
	  break;
      case RL2_SAMPLE_UINT16:
	  fprintf (stderr, "Sample Type: UINT16\n");
	  break;
      case RL2_SAMPLE_INT32:
	  fprintf (stderr, "Sample Type: INT32\n");
	  break;
      case RL2_SAMPLE_UINT32:
	  fprintf (stderr, "Sample Type: UINT32\n");
	  break;
      case RL2_SAMPLE_FLOAT:
	  fprintf (stderr, "Sample Type: FLOAT\n");
	  break;
      case RL2_SAMPLE_DOUBLE:
	  fprintf (stderr, "Sample Type: DOUBLE\n");
	  break;
      default:
	  fprintf (stderr, "*** ERROR *** unknown sample type\n");
	  err = 1;
	  break;
      };
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  fprintf (stderr, "Pixel Type: MONOCHROME\n");
	  num_bands = 1;
	  break;
      case RL2_PIXEL_PALETTE:
	  fprintf (stderr, "Pixel Type: PALETTE\n");
	  num_bands = 1;
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  fprintf (stderr, "Pixel Type: GRAYSCALE\n");
	  num_bands = 1;
	  break;
      case RL2_PIXEL_RGB:
	  fprintf (stderr, "Pixel Type: RGB\n");
	  num_bands = 3;
	  break;
      case RL2_PIXEL_MULTIBAND:
	  fprintf (stderr, "Pixel Type: MULTIBAND\n");
	  break;
      case RL2_PIXEL_DATAGRID:
	  fprintf (stderr, "Pixel Type: DATAGRID\n");
	  num_bands = 1;
	  break;
      default:
	  fprintf (stderr, "*** ERROR *** unknown pixel type\n");
	  err = 1;
	  break;
      };
    if (num_bands == RL2_BANDS_UNKNOWN)
	fprintf (stderr, "*** ERROR *** unkown number of Bands\n");
    else
	fprintf (stderr, "Number of Bands: %d\n", num_bands);
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  fprintf (stderr, "Compression: NONE (uncompressed)\n");
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  fprintf (stderr, "Compression: DEFLATE (zip, lossless)\n");
	  break;
      case RL2_COMPRESSION_LZMA:
	  fprintf (stderr, "Compression: LZMA (7-zip, lossless)\n");
	  break;
      case RL2_COMPRESSION_GIF:
	  fprintf (stderr, "Compression: GIF, lossless\n");
	  break;
      case RL2_COMPRESSION_PNG:
	  fprintf (stderr, "Compression: PNG, lossless\n");
	  break;
      case RL2_COMPRESSION_JPEG:
	  fprintf (stderr, "Compression: JPEG (lossy)\n");
	  if (quality < 0)
	      quality = 0;
	  if (quality > 100)
	      quality = 100;
	  fprintf (stderr, "Compression Quality: %d\n", quality);
	  break;
      case RL2_COMPRESSION_LOSSY_WEBP:
	  fprintf (stderr, "Compression: WEBP (lossy)\n");
	  if (quality < 0)
	      quality = 0;
	  if (quality > 100)
	      quality = 100;
	  fprintf (stderr, "Compression Quality: %d\n", quality);
	  break;
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  fprintf (stderr, "Compression: WEBP, lossless\n");
	  break;
      default:
	  fprintf (stderr, "*** ERROR *** unknown compression\n");
	  err = 1;
	  break;
      };
    fprintf (stderr, "Tile size (pixels): %d x %d\n", tile_width, tile_height);
    if (srid <= 0)
      {
	  fprintf (stderr, "*** ERROR *** undefined SRID\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Srid: %d\n", srid);
    if (x_res <= 0.0)
      {
	  fprintf (stderr, "*** ERROR *** invalid X pixel size\n");
	  err = 1;
      }
    if (y_res <= 0.0)
      {
	  fprintf (stderr, "*** ERROR *** invalid Y pixel size\n");
	  err = 1;
      }
    if (x_res > 0.0 && x_res > 0.0)
	fprintf (stderr, "Pixel size: X=%1.8f Y=%1.8f\n", x_res, y_res);
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_import_args (const char *db_path, const char *src_path,
		   const char *coverage, const char *section,
		   const char *wf_path, int pyramidize)
{
/* checking/printing IMPORT args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is IMPORT\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    if (src_path == NULL)
      {
	  fprintf (stderr,
		   "*** ERROR *** no input Source path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "input Source path: %s\n", src_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    if (section == NULL)
	fprintf (stderr, "Section: from file name\n");
    else
	fprintf (stderr, "Section: %s\n", section);
    if (wf_path == NULL)
	fprintf (stderr, "WorldFile path: none specified\n");
    else
	fprintf (stderr, "WorldFile path: %s\n", wf_path);
    if (pyramidize)
	fprintf (stderr, "Immediately building Pyramid Levels\n");
    else
	fprintf (stderr, "Ignoring Pyramid Levels for now\n");
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_drop_args (const char *db_path, const char *coverage)
{
/* checking/printing DROP args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is DROP\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_delete_args (const char *db_path, const char *coverage,
		   const char *section)
{
/* checking/printing DELETE args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is DELETE\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    if (section == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Section's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Section: %s\n", section);
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_pyramidize_args (const char *db_path, const char *coverage,
		       const char *section, int force)
{
/* checking/printing PYRAMIDIZE args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is PYRAMIDIZE\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    if (section == NULL)
	fprintf (stderr, "Section: All Sections\n");
    else
	fprintf (stderr, "Section: %s\n", section);
    if (force)
	fprintf (stderr, "Unconditionally rebuilding all Pyramid Levels\n");
    else
	fprintf (stderr, "Only building missing Pyramid Levels\n");
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_list_args (const char *db_path, const char *coverage, const char *section)
{
/* checking/printing LIST args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is LIST\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    if (section == NULL)
	fprintf (stderr, "Section: All Sections\n");
    else
	fprintf (stderr, "Section: %s\n", section);
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_catalog_args (const char *db_path)
{
/* checking/printing CATALOG args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is CATALOG\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_check_args (const char *db_path, const char *coverage,
		  const char *section)
{
/* checking/printing CHECK args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is CHECK\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "DB path: %s\n", db_path);
    if (coverage == NULL)
	fprintf (stderr, "Coverage: All Coverages\n");
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    if (section == NULL)
	fprintf (stderr, "Section: All Sections\n");
    else
	fprintf (stderr, "Section: %s\n", section);
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static void
do_help ()
{
/* printing the argument list */
    fprintf (stderr, "\n\nusage: rl2_tool MODE [ ARGLIST ]\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr,
	     "-h or --help                    print this help message\n");
/* MODE = CREATE */
    fprintf (stderr, "\nmode: CREATE\n");
    fprintf (stderr, "will create a new RasterLite2 Raster Coverage\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
    fprintf (stderr,
	     "-smp or --sample-type keyword   Sample Type keyword (see list)\n");
    fprintf (stderr,
	     "-pxl or --pixel-type  keyword   Pixel Type keyword (see list)\n");
    fprintf (stderr, "-bds or --num-bands   integer   Number of Bands\n");
    fprintf (stderr,
	     "-cpr or --compression keyword   Compression keyword (see list)\n");
    fprintf (stderr,
	     "-qty or --quality     integer   Compression Quality [0-100]\n");
    fprintf (stderr, "-tlw or --tile-width  integer   Tile Width [pixels]\n");
    fprintf (stderr, "-tlh or --tile-height integer   Tile Height [pixels]\n");
    fprintf (stderr, "-srid or --srid       integer   SRID value\n");
    fprintf (stderr,
	     "-res or --resolution  number    pixel resolution(X and Y)\n");
    fprintf (stderr,
	     "-xres or --x-resol    number    pixel resolution(X specific)\n");
    fprintf (stderr,
	     "-yres or --y-resol    number    pixel resolution(Y specific)\n\n");
    fprintf (stderr, "SampleType Keywords:\n");
    fprintf (stderr, "----------------------------------\n");
    fprintf (stderr,
	     "1-BIT 2-BIT 4-BIT INT8 UINT8 INT16 UINT16\n"
	     " INT32 UINT32 FLOAT DOUBLE\n\n");
    fprintf (stderr, "PixelType Keywords:\n");
    fprintf (stderr, "----------------------------------\n");
    fprintf (stderr, "MONOCHROME PALETTE GRAYSCALE RGB MULTIBAND DATAGRID\n\n");
    fprintf (stderr, "Compression Keywords:\n");
    fprintf (stderr, "----------------------------------\n");
    fprintf (stderr,
	     "NONE DEFLATE LZMA GIF PNG JPEG LOSSY_WEBP LOSSLESS_WEBP\n\n");
/* MODE = DROP */
    fprintf (stderr, "\nmode: DROP\n");
    fprintf (stderr, "will drop an existing RasterLite2 Raster Coverage\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr, "-cov or --coverage    string    Coverage's name\n\n");
/* MODE = IMPORT */
    fprintf (stderr, "\nmode: IMPORT\n");
    fprintf (stderr, "will create a new Raster Section by importing an\n");
    fprintf (stderr, "external image or raster file\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr,
	     "-src or --src-path    pathname  input Image/Raster path\n");
    fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
    fprintf (stderr,
	     "-sec or --section     string    optional: Section's name\n");
    fprintf (stderr,
	     "-wf or --wf-path      pathname  optional: WorldFile path\n");
    fprintf (stderr,
	     "-pyr or --pyramidize            immediately build Pyramid levels\n\n");
/* MODE = DELETE */
    fprintf (stderr, "\nmode: DELETE\n");
    fprintf (stderr, "will delete a Raster Section\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
    fprintf (stderr, "-sec or --section     string    Section's name\n\n");
/* MODE = PYRAMIDIZE */
    fprintf (stderr, "\nmode: PYRAMIDIZE\n");
    fprintf (stderr,
	     "will (re)build all Pyramid levels supporting a Coverage\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
    fprintf (stderr,
	     "-sec or --section     string    optional: Section's name\n");
    fprintf (stderr,
	     "                                default is \"All Sections\"\n");
    fprintf (stderr,
	     "-f or --force                   optional: rebuilds from scratch\n\n");
/* MODE = LIST */
    fprintf (stderr, "\nmode: LIST\n");
    fprintf (stderr, "will list Raster Sections within a Coverage\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
    fprintf (stderr,
	     "-sec or --section     string    optional: Section's name\n");
    fprintf (stderr,
	     "                                default is \"All Sections\"\n");
/* MODE = CATALOG */
    fprintf (stderr, "\nmode: CATALOG\n");
    fprintf (stderr, "will list all Coverages from within a RasterLite2 DB\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n\n");
/* MODE = CHECK */
    fprintf (stderr, "\nmode: CHECK\n");
    fprintf (stderr, "will check a RasterLite2 DB for validity\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr,
	     "-cov or --coverage    string    optional: Coverage's name\n");
    fprintf (stderr,
	     "                                default is \"All Coverages\"\n");
    fprintf (stderr,
	     "-sec or --section     string    optional: Section's name\n");
    fprintf (stderr,
	     "                                default is \"All Sections\"\n\n");
/* DB options */
    fprintf (stderr, "\noptional DB specific settings:\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr,
	     "-cs or --cache-size    num      DB cache size (how many pages)\n");
    fprintf (stderr,
	     "-m or --in-memory               using IN-MEMORY database\n");
    fprintf (stderr,
	     "-jo or --journal-off            unsafe [but faster] mode\n");
}

int
main (int argc, char *argv[])
{
/* the MAIN function simply perform arguments checking */
    sqlite3 *handle;
    int ret;
    char *sql_err = NULL;
    int i;
    int next_arg = ARG_NONE;
    const char *db_path = NULL;
    const char *src_path = NULL;
    const char *wf_path = NULL;
    const char *coverage = NULL;
    const char *section = NULL;
    int sample = RL2_SAMPLE_UNKNOWN;
    int pixel = RL2_PIXEL_UNKNOWN;
    int compression = RL2_COMPRESSION_NONE;
    int num_bands = RL2_BANDS_UNKNOWN;
    int quality = 80;
    int tile_width = 256;
    int tile_height = 256;
    double x_res = 0.0;
    double y_res = 0.0;
    int srid = -1;
    int pyramidize = 0;
    int force_pyramid = 0;
    int in_memory = 0;
    int cache_size = 0;
    int journal_off = 0;
    int error = 0;
    int mode = ARG_NONE;
    void *cache;
    void *cache_mem;

    if (argc >= 2)
      {
	  /* extracting the MODE */
	  if (strcasecmp (argv[1], "CREATE") == 0)
	      mode = ARG_MODE_CREATE;
	  if (strcasecmp (argv[1], "DROP") == 0)
	      mode = ARG_MODE_DROP;
	  if (strcasecmp (argv[1], "IMPORT") == 0)
	      mode = ARG_MODE_IMPORT;
	  if (strcasecmp (argv[1], "DELETE") == 0)
	      mode = ARG_MODE_DELETE;
	  if (strcasecmp (argv[1], "PYRAMIDIZE") == 0)
	      mode = ARG_MODE_PYRAMIDIZE;
	  if (strcasecmp (argv[1], "LIST") == 0)
	      mode = ARG_MODE_LIST;
	  if (strcasecmp (argv[1], "CATALOG") == 0)
	      mode = ARG_MODE_CATALOG;
	  if (strcasecmp (argv[1], "CHECK") == 0)
	      mode = ARG_MODE_CHECK;
      }

    for (i = 2; i < argc; i++)
      {
	  /* parsing the invocation arguments */
	  if (next_arg != ARG_NONE)
	    {
		switch (next_arg)
		  {
		  case ARG_DB_PATH:
		      db_path = argv[i];
		      break;
		  case ARG_SRC_PATH:
		      src_path = argv[i];
		      break;
		  case ARG_WF_PATH:
		      wf_path = argv[i];
		      break;
		  case ARG_COVERAGE:
		      coverage = argv[i];
		      break;
		  case ARG_SECTION:
		      section = argv[i];
		      break;
		  case ARG_SAMPLE:
		      if (strcasecmp (argv[i], "1-BIT") == 0)
			  sample = RL2_SAMPLE_1_BIT;
		      if (strcasecmp (argv[i], "2-BIT") == 0)
			  sample = RL2_SAMPLE_2_BIT;
		      if (strcasecmp (argv[i], "4-BIT") == 0)
			  sample = RL2_SAMPLE_4_BIT;
		      if (strcasecmp (argv[i], "INT8") == 0)
			  sample = RL2_SAMPLE_INT8;
		      if (strcasecmp (argv[i], "UINT8") == 0)
			  sample = RL2_SAMPLE_UINT8;
		      if (strcasecmp (argv[i], "INT16") == 0)
			  sample = RL2_SAMPLE_INT16;
		      if (strcasecmp (argv[i], "UINT16") == 0)
			  sample = RL2_SAMPLE_UINT16;
		      if (strcasecmp (argv[i], "INT32") == 0)
			  sample = RL2_SAMPLE_INT32;
		      if (strcasecmp (argv[i], "UINT32") == 0)
			  sample = RL2_SAMPLE_UINT32;
		      if (strcasecmp (argv[i], "INT8") == 0)
			  sample = RL2_SAMPLE_INT8;
		      if (strcasecmp (argv[i], "UINT8") == 0)
			  sample = RL2_SAMPLE_UINT8;
		      if (strcasecmp (argv[i], "FLOAT") == 0)
			  sample = RL2_SAMPLE_FLOAT;
		      if (strcasecmp (argv[i], "DOUBLE") == 0)
			  sample = RL2_SAMPLE_DOUBLE;
		      break;
		  case ARG_PIXEL:
		      if (strcasecmp (argv[i], "MONOCHROME") == 0)
			  pixel = RL2_PIXEL_MONOCHROME;
		      if (strcasecmp (argv[i], "PALETTE") == 0)
			  pixel = RL2_PIXEL_PALETTE;
		      if (strcasecmp (argv[i], "GRAYSCALE") == 0)
			  pixel = RL2_PIXEL_GRAYSCALE;
		      if (strcasecmp (argv[i], "RGB") == 0)
			  pixel = RL2_PIXEL_RGB;
		      if (strcasecmp (argv[i], "MULTIBAND") == 0)
			  pixel = RL2_PIXEL_MULTIBAND;
		      if (strcasecmp (argv[i], "DATAGRID") == 0)
			  pixel = RL2_PIXEL_DATAGRID;
		      break;
		  case ARG_NUM_BANDS:
		      num_bands = atoi (argv[i]);
		      break;
		  case ARG_COMPRESSION:
		      if (strcasecmp (argv[i], "NONE") == 0)
			  compression = RL2_COMPRESSION_NONE;
		      if (strcasecmp (argv[i], "DEFLATE") == 0)
			  compression = RL2_COMPRESSION_DEFLATE;
		      if (strcasecmp (argv[i], "LZMA") == 0)
			  compression = RL2_COMPRESSION_LZMA;
		      if (strcasecmp (argv[i], "GIF") == 0)
			  compression = RL2_COMPRESSION_GIF;
		      if (strcasecmp (argv[i], "PNG") == 0)
			  compression = RL2_COMPRESSION_PNG;
		      if (strcasecmp (argv[i], "JPEG") == 0)
			  compression = RL2_COMPRESSION_JPEG;
		      if (strcasecmp (argv[i], "LOSSY_WEBP") == 0)
			  compression = RL2_COMPRESSION_LOSSY_WEBP;
		      if (strcasecmp (argv[i], "LOSSLESS_WEBP") == 0)
			  compression = RL2_COMPRESSION_LOSSLESS_WEBP;
		      break;
		  case ARG_QUALITY:
		      quality = atoi (argv[i]);
		      break;
		  case ARG_TILE_WIDTH:
		      tile_width = atoi (argv[i]);
		      break;
		  case ARG_TILE_HEIGHT:
		      tile_height = atoi (argv[i]);
		      break;
		  case ARG_SRID:
		      srid = atoi (argv[i]);
		      break;
		  case ARG_RESOLUTION:
		      x_res = atof (argv[i]);
		      y_res = x_res;
		      break;
		  case ARG_X_RESOLUTION:
		      x_res = atof (argv[i]);
		      break;
		  case ARG_Y_RESOLUTION:
		      y_res = atof (argv[i]);
		      break;
		  case ARG_CACHE_SIZE:
		      cache_size = atoi (argv[i]);
		      break;
		  };
		next_arg = ARG_NONE;
		continue;
	    }

	  if (strcasecmp (argv[i], "--help") == 0
	      || strcmp (argv[i], "-h") == 0)
	    {
		do_help ();
		return -1;
	    }
	  if (strcmp (argv[i], "-db") == 0
	      || strcasecmp (argv[i], "--db-path") == 0)
	    {
		next_arg = ARG_DB_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-src") == 0
	      || strcasecmp (argv[i], "--src-path") == 0)
	    {
		next_arg = ARG_SRC_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-wf") == 0
	      || strcasecmp (argv[i], "--wf-path") == 0)
	    {
		next_arg = ARG_WF_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-cov") == 0
	      || strcasecmp (argv[i], "--coverage") == 0)
	    {
		next_arg = ARG_COVERAGE;
		continue;
	    }
	  if (strcmp (argv[i], "-sec") == 0
	      || strcasecmp (argv[i], "--section") == 0)
	    {
		next_arg = ARG_SECTION;
		continue;
	    }
	  if (strcmp (argv[i], "-smp") == 0
	      || strcasecmp (argv[i], "--sample-type") == 0)
	    {
		next_arg = ARG_SAMPLE;
		continue;
	    }
	  if (strcmp (argv[i], "-pxl") == 0
	      || strcasecmp (argv[i], "--pixel-type") == 0)
	    {
		next_arg = ARG_PIXEL;
		continue;
	    }
	  if (strcmp (argv[i], "-bds") == 0
	      || strcasecmp (argv[i], "--num-bands") == 0)
	    {
		next_arg = ARG_NUM_BANDS;
		continue;
	    }
	  if (strcmp (argv[i], "-cpr") == 0
	      || strcasecmp (argv[i], "--compression") == 0)
	    {
		next_arg = ARG_COMPRESSION;
		continue;
	    }
	  if (strcmp (argv[i], "-qty") == 0
	      || strcasecmp (argv[i], "--quality") == 0)
	    {
		next_arg = ARG_QUALITY;
		continue;
	    }
	  if (strcmp (argv[i], "-tlw") == 0
	      || strcasecmp (argv[i], "--tile-width") == 0)
	    {
		next_arg = ARG_TILE_WIDTH;
		continue;
	    }
	  if (strcmp (argv[i], "-tlh") == 0
	      || strcasecmp (argv[i], "--tile-height") == 0)
	    {
		next_arg = ARG_TILE_HEIGHT;
		continue;
	    }
	  if (strcmp (argv[i], "-srid") == 0
	      || strcasecmp (argv[i], "--srid") == 0)
	    {
		next_arg = ARG_SRID;
		continue;
	    }
	  if (strcmp (argv[i], "-res") == 0
	      || strcasecmp (argv[i], "--resolution") == 0)
	    {
		next_arg = ARG_RESOLUTION;
		continue;
	    }
	  if (strcmp (argv[i], "-xres") == 0
	      || strcasecmp (argv[i], "--x-resol") == 0)
	    {
		next_arg = ARG_X_RESOLUTION;
		continue;
	    }
	  if (strcmp (argv[i], "-yres") == 0
	      || strcasecmp (argv[i], "--y-resol") == 0)
	    {
		next_arg = ARG_Y_RESOLUTION;
		continue;
	    }
	  if (strcmp (argv[i], "-f") == 0
	      || strcasecmp (argv[i], "--force") == 0)
	    {
		force_pyramid = 1;
		continue;
	    }
	  if (strcmp (argv[i], "-pyr") == 0
	      || strcasecmp (argv[i], "--pyramidize") == 0)
	    {
		pyramidize = 1;
		continue;
	    }
	  if (strcasecmp (argv[i], "--cache-size") == 0
	      || strcmp (argv[i], "-cs") == 0)
	    {
		next_arg = ARG_CACHE_SIZE;
		continue;
	    }
	  if (strcmp (argv[i], "-m") == 0
	      || strcasecmp (argv[i], "--in-memory") == 0)
	    {
		in_memory = 1;
		next_arg = ARG_NONE;
		continue;
	    }
	  if (strcmp (argv[i], "-jo") == 0
	      || strcasecmp (argv[i], "--journal-off") == 0)
	    {
		journal_off = 1;
		next_arg = ARG_NONE;
		continue;
	    }
	  fprintf (stderr, "unknown argument: %s\n", argv[i]);
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }

/* checking the arguments */
    switch (mode)
      {
      case ARG_MODE_CREATE:
	  error =
	      check_create_args (db_path, coverage, sample, pixel, num_bands,
				 compression, quality, tile_width, tile_height,
				 srid, x_res, y_res);
	  break;
      case ARG_MODE_DROP:
	  error = check_drop_args (db_path, coverage);
	  break;
      case ARG_MODE_IMPORT:
	  error =
	      check_import_args (db_path, src_path, coverage, section, wf_path,
				 pyramidize);
	  break;
      case ARG_MODE_DELETE:
	  error = check_delete_args (db_path, coverage, section);
	  break;
      case ARG_MODE_PYRAMIDIZE:
	  error =
	      check_pyramidize_args (db_path, coverage, section, force_pyramid);
	  break;
      case ARG_MODE_LIST:
	  error = check_list_args (db_path, coverage, section);
	  break;
      case ARG_MODE_CATALOG:
	  error = check_catalog_args (db_path);
	  break;
      case ARG_MODE_CHECK:
	  error = check_check_args (db_path, coverage, section);
	  break;
      default:
	  fprintf (stderr, "did you forget setting some request MODE ?\n");
	  error = 1;
	  break;
      };

    if (error)
      {
	  do_help ();
	  return -1;
      }

/* opening the DB */
    if (in_memory)
	cache_size = 0;
    cache = spatialite_alloc_connection ();
    open_db (db_path, &handle, cache_size, cache);
    if (!handle)
	return -1;
    if (in_memory)
      {
	  /* loading the DB in-memory */
	  sqlite3 *mem_db_handle;
	  sqlite3_backup *backup;
	  int ret;
	  ret =
	      sqlite3_open_v2 (":memory:", &mem_db_handle,
			       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			       NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "cannot open 'MEMORY-DB': %s\n",
			 sqlite3_errmsg (mem_db_handle));
		sqlite3_close (mem_db_handle);
		return -1;
	    }
	  backup = sqlite3_backup_init (mem_db_handle, "main", handle, "main");
	  if (!backup)
	    {
		fprintf (stderr, "cannot load 'MEMORY-DB'\n");
		sqlite3_close (handle);
		sqlite3_close (mem_db_handle);
		return -1;
	    }
	  while (1)
	    {
		ret = sqlite3_backup_step (backup, 1024);
		if (ret == SQLITE_DONE)
		    break;
	    }
	  ret = sqlite3_backup_finish (backup);
	  sqlite3_close (handle);
	  handle = mem_db_handle;
	  printf ("\nusing IN-MEMORY database\n");
	  spatialite_cleanup_ex (cache);
	  cache_mem = spatialite_alloc_connection ();
	  spatialite_init_ex (handle, cache_mem, 0);
      }

/* properly setting up the connection */
    if (!set_connection (handle, journal_off))
	goto stop;

/* the complete operation is handled as an unique SQL Transaction */
    ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, &sql_err);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "BEGIN TRANSACTION error: %s\n", sql_err);
	  sqlite3_free (sql_err);
	  goto stop;
      }

/* executing the required command */
    switch (mode)
      {
      case ARG_MODE_CREATE:
	  ret =
	      exec_create (handle, coverage, sample, pixel,
			   num_bands, compression, quality, tile_width,
			   tile_height, srid, x_res, y_res);
	  break;
      case ARG_MODE_DROP:
	  ret = exec_drop (handle, coverage);
	  break;
      case ARG_MODE_IMPORT:
	  ret =
	      exec_import (handle, src_path, coverage, section,
			   wf_path, pyramidize);
	  break;
      case ARG_MODE_DELETE:
	  ret = exec_delete (handle, coverage, section);
	  break;
      case ARG_MODE_PYRAMIDIZE:
	  ret = exec_pyramidize (handle, coverage, section, force_pyramid);
	  break;
/*
	     case ARG_MODE_LIST:
	     ret = exec_list(handle, db_path, coverage, section);
	     break;
	     case ARG_MODE_CATALOG:
	     ret = exec_catalog(handle, db_path);
	     break;
	     case ARG_MODE_CHECK:
	     ret = exec_check(handle, db_path, coverage, section);
	     break;
	   */
      };

    if (ret)
      {
	  /* committing the still pending SQL Transaction */
	  const char *op_name = "UNKNOWN";
	  ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, &sql_err);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "COMMIT TRANSACTION error: %s\n", sql_err);
		sqlite3_free (sql_err);
	    }
	  switch (mode)
	    {
	    case ARG_MODE_CREATE:
		op_name = "CREATE";
		break;
	    case ARG_MODE_DROP:
		op_name = "DROP";
		break;
	    case ARG_MODE_IMPORT:
		op_name = "IMPORT";
		break;
	    case ARG_MODE_DELETE:
		op_name = "DELETE";
		break;
	    case ARG_MODE_PYRAMIDIZE:
		op_name = "PYRAMIDIZE";
		break;
	    case ARG_MODE_LIST:
		op_name = "LIST";
		break;
	    case ARG_MODE_CATALOG:
		op_name = "CATALOG";
		break;
	    case ARG_MODE_CHECK:
		op_name = "CHECK";
		break;
	    };
	  fprintf (stderr, "\nOperation %s succesfully completed\n", op_name);
      }
    else
      {
	  /* invalidating the still pending SQL Transaction */
	  fprintf (stderr,
		   "\nrestoring the DB to its previous state (ROLLBACK)\n");
	  ret = sqlite3_exec (handle, "ROLLBACK", NULL, NULL, &sql_err);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "ROLLBACK TRANSACTION error: %s\n", sql_err);
		sqlite3_free (sql_err);
	    }
      }

    if (in_memory)
      {
	  /* exporting the in-memory DB to filesystem */
	  sqlite3 *disk_db_handle;
	  sqlite3_backup *backup;
	  int ret;
	  printf ("\nexporting IN_MEMORY database ... wait please ...\n");
	  ret =
	      sqlite3_open_v2 (db_path, &disk_db_handle,
			       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			       NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "cannot open '%s': %s\n", db_path,
			 sqlite3_errmsg (disk_db_handle));
		sqlite3_close (disk_db_handle);
		return -1;
	    }
	  backup = sqlite3_backup_init (disk_db_handle, "main", handle, "main");
	  if (!backup)
	    {
		fprintf (stderr, "Backup failure: 'MEMORY-DB' wasn't saved\n");
		sqlite3_close (handle);
		sqlite3_close (disk_db_handle);
		return -1;
	    }
	  while (1)
	    {
		ret = sqlite3_backup_step (backup, 1024);
		if (ret == SQLITE_DONE)
		    break;
	    }
	  ret = sqlite3_backup_finish (backup);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache_mem);
	  handle = disk_db_handle;
	  printf ("\tIN_MEMORY database succesfully exported\n");
      }

  stop:
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    return 0;
}
