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
#include <float.h>
#include <limits.h>

#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__)
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#endif

#include <rasterlite2/rasterlite2.h>
#include <rasterlite2/rl2tiff.h>
#include <rasterlite2/rl2graphics.h>

#include <spatialite/gaiaaux.h>
#include <spatialite.h>

#define ARG_NONE		0
#define ARG_MODE_CREATE		1
#define ARG_MODE_DROP		2
#define ARG_MODE_IMPORT		3
#define ARG_MODE_EXPORT		4
#define ARG_MODE_DELETE		5
#define ARG_MODE_PYRAMIDIZE	6
#define ARG_MODE_LIST		7
#define ARG_MODE_CATALOG	8
#define ARG_MODE_MAP		9
#define ARG_MODE_CHECK		10

#define ARG_DB_PATH		10
#define ARG_SRC_PATH		11
#define ARG_DST_PATH		12
#define ARG_DIR_PATH		13
#define ARG_FILE_EXT		14
#define ARG_COVERAGE		15
#define ARG_SECTION		16
#define ARG_SAMPLE		17
#define ARG_PIXEL		18
#define ARG_NUM_BANDS		19
#define ARG_COMPRESSION		20
#define ARG_QUALITY		21
#define ARG_TILE_WIDTH		22
#define ARG_TILE_HEIGHT		23
#define ARG_IMG_WIDTH		24
#define ARG_IMG_HEIGHT		25
#define ARG_SRID		26
#define ARG_RESOLUTION		27
#define ARG_X_RESOLUTION	28
#define ARG_Y_RESOLUTION	29
#define ARG_MINX	30
#define ARG_MINY	31
#define ARG_MAXX	32
#define ARG_MAXY	33
#define ARG_CX	34
#define ARG_CY	35

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
formatFloat (double value)
{
/* nicely formatting a float value */
    int i;
    int len;
    char *fmt = sqlite3_mprintf ("%1.24f", value);
    len = strlen (fmt);
    for (i = len - 1; i >= 0; i--)
      {
	  if (fmt[i] == '0')
	      fmt[i] = '\0';
	  else
	      break;
      }
    len = strlen (fmt);
    if (fmt[len - 1] == '.')
	fmt[len] = '0';
    return fmt;
}

static char *
formatFloat2 (double value)
{
/* nicely formatting a float value (2 decimals) */
    char *fmt = sqlite3_mprintf ("%1.2f", value);
    return fmt;
}

static char *
formatFloat6 (double value)
{
/* nicely formatting a float value (6 decimals) */
    char *fmt = sqlite3_mprintf ("%1.2f", value);
    return fmt;
}

static char *
formatLong (double value)
{
/* nicely formatting a Longitude */
    if (value >= -180.0 && value <= 180.0)
	return formatFloat6 (value);
    return formatFloat2 (value);
}

static char *
formatLat (double value)
{
/* nicely formatting a Latitude */
    if (value >= -90.0 && value <= 90.0)
	return formatFloat6 (value);
    return formatFloat2 (value);
}

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
	     unsigned char sample, unsigned char pixel, unsigned char num_bands,
	     unsigned char compression, int quality, unsigned short tile_width,
	     unsigned short tile_height, int srid, double x_res, double y_res)
{
/* performing CREATE */
    if (rl2_create_dbms_coverage
	(handle, coverage, sample, pixel, num_bands, compression, quality,
	 tile_width, tile_height, srid, x_res, y_res) != RL2_OK)
	return 0;

    fprintf (stderr, "\rRaster Coverage \"%s\" successfully created\n",
	     coverage);
    return 1;
}

static int
stampa_statistiche (rl2RasterStatisticsPtr stats)
{
    double no_data;
    double count;
    double mean;
    double variance;
    double stddev;
    double min;
    double max;
    unsigned char sample_type;
    unsigned char num_bands;
    unsigned char ib;

    if (rl2_get_raster_statistics_summary
	(stats, &no_data, &count, &sample_type, &num_bands) != RL2_OK)
      {
	  fprintf (stderr, "SUMMARY ERROR\n");
	  return;
      }
    fprintf (stderr, "no_data=%1.6f\n", no_data);
    fprintf (stderr, "  count=%1.6f\n", count);
    fprintf (stderr, "type=%02x bands=%02x\n", sample_type, num_bands);

    for (ib = 0; ib < num_bands; ib++)
      {
	  if (rl2_get_band_statistics
	      (stats, ib, &min, &max, &mean, &variance, &stddev) != RL2_OK)
	    {
		fprintf (stderr, "BAND ERROR %d\n", ib);
		return;
	    }
	  fprintf (stderr, "band%d      min=%1.6f\n", ib, min);
	  fprintf (stderr, "band%d      max=%1.6f\n", ib, max);
	  fprintf (stderr, "band%d     mean=%1.6f\n", ib, mean);
	  fprintf (stderr, "band%d variance=%1.6f\n", ib, variance);
	  fprintf (stderr, "band%d   stddev=%1.6f\n", ib, stddev);
      }
}

static int
do_import_file (sqlite3 * handle, const char *src_path,
		rl2CoveragePtr cvg, const char *section, int worldfile,
		int force_srid, int pyramidize, unsigned char sample_type,
		unsigned char pixel_type, unsigned char num_bands,
		unsigned short tile_w, unsigned short tile_h,
		unsigned char compression, int quality,
		sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
		sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
		sqlite3_stmt * stmt_upd_sect)
{
/* importing a single Source file */
    int ret;
    rl2TiffOriginPtr origin = NULL;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr aux_palette = NULL;
    rl2RasterStatisticsPtr stats;
    rl2RasterStatisticsPtr section_stats = NULL;
    int row;
    int col;
    unsigned short width;
    unsigned short height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    unsigned char *blob_stats;
    int blob_stats_sz;
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
    char *dumb1;
    char *dumb2;
    int bnd;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    sqlite3_int64 section_id;
    sqlite3_int64 tile_id;

    if (worldfile)
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_WORLDFILE, force_srid,
				    sample_type, pixel_type, num_bands);
    else
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_GEOTIFF, force_srid,
				    sample_type, pixel_type, num_bands);
    if (origin == NULL)
	goto error;

    printf ("Importing: %s\n", rl2_get_tiff_origin_path (origin));
    printf ("------------------\n");
    ret = rl2_get_tiff_origin_size (origin, &width, &height);
    if (ret == RL2_OK)
	printf ("    Image Size (pixels): %d x %d\n", width, height);
    ret = rl2_get_tiff_origin_srid (origin, &srid);
    if (ret == RL2_OK)
	printf ("                   SRID: %d\n", srid);
    ret = rl2_get_tiff_origin_extent (origin, &minx, &miny, &maxx, &maxy);
    if (ret == RL2_OK)
      {
	  dumb1 = formatLong (minx);
	  dumb2 = formatLat (miny);
	  printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
	  dumb1 = formatLong (maxx);
	  dumb2 = formatLat (maxy);
	  printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    ret = rl2_get_tiff_origin_resolution (origin, &res_x, &res_y);
    if (ret == RL2_OK)
      {
	  dumb1 = formatFloat (res_x);
	  dumb2 = formatFloat (res_y);
	  printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }

    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  /* remapping the Palette */
	  if (rl2_check_dbms_palette (handle, cvg, origin) != RL2_OK)
	    {
		fprintf (stderr, "Mismatching Palette !!!\n");
		goto error;
	    }
      }

    if (rl2_eval_tiff_origin_compatibility (cvg, origin) != RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch\n");
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
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;

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
		/* INSERTing the tile */
		aux_palette =
		    rl2_clone_palette (rl2_get_raster_palette (raster));
		stats = rl2_get_raster_statistics
		    (blob_odd, blob_odd_sz, blob_even, blob_even_sz,
		     aux_palette);
		if (aux_palette != NULL)
		    rl2_destroy_palette (aux_palette);
		aux_palette = NULL;
		rl2_aggregate_raster_statistics (stats, section_stats);
		sqlite3_reset (stmt_tils);
		sqlite3_clear_bindings (stmt_tils);
		sqlite3_bind_int64 (stmt_tils, 1, section_id);
		tile_maxx = tile_minx + ((double) tile_w * res_x);
		if (tile_maxx > maxx)
		    tile_maxx = maxx;
		tile_miny = tile_maxy - ((double) tile_h * res_y);
		if (tile_miny < miny)
		    tile_miny = miny;
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
		sqlite3_bind_int64 (stmt_data, 1, tile_id);
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
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

    stampa_statistiche (section_stats);

/* updating the Section's Statistics */
    sqlite3_reset (stmt_upd_sect);
    sqlite3_clear_bindings (stmt_upd_sect);
    rl2_serialize_dbms_raster_statistics (section_stats, &blob_stats,
					  &blob_stats_sz);
    sqlite3_bind_blob (stmt_upd_sect, 1, blob_stats, blob_stats_sz, free);
    sqlite3_bind_int64 (stmt_upd_sect, 2, section_id);
    ret = sqlite3_step (stmt_upd_sect);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "UPDATE sections; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }

    rl2_destroy_tiff_origin (origin);
    rl2_destroy_raster_statistics (section_stats);

    return 1;

  error:
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    if (origin != NULL)
	rl2_destroy_tiff_origin (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (aux_palette != NULL)
	rl2_destroy_palette (aux_palette);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

static int
check_extension_match (const char *file_name, const char *file_ext)
{
/* checks the file extension */
    const char *mark = NULL;
    const char *p = file_name;
    while (*p != '\0')
      {
	  if (*p == '.')
	      mark = p;
	  p++;
      }
    if (mark == NULL)
	return 0;
    if (strcmp (mark, file_ext) == 0)
	return 1;
    return 0;
}

static int
do_import_dir (sqlite3 * handle, const char *dir_path, const char *file_ext,
	       rl2CoveragePtr cvg, const char *section, int worldfile,
	       int force_srid, int pyramidize, unsigned char sample_type,
	       unsigned char pixel_type, unsigned char num_bands,
	       unsigned short tile_w, unsigned short tile_h,
	       unsigned char compression, int quality, sqlite3_stmt * stmt_data,
	       sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
	       sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect)
{
/* importing a whole directory */
#if defined(_WIN32) && !defined(__MINGW32__)
/* Visual Studio .NET */
    struct _finddata_t c_file;
    intptr_t hFile;
    int cnt = 0;
    char *search;
    char *path;
    int ret;
    if (_chdir (dir_path) < 0)
	return 0;
    search = sqlite3_mprintf ("*%s", file_ext);
    if ((hFile = _findfirst (search, &c_file)) == -1L)
	;
    else
      {
	  while (1)
	    {
		if ((c_file.attrib & _A_RDONLY) == _A_RDONLY
		    || (c_file.attrib & _A_NORMAL) == _A_NORMAL)
		  {
		      path = sqlite3_mprintf ("%s/%s", dir_path, c_file.name);
		      ret =
			  do_import_file (handle, path, cvg, section, worldfile,
					  force_srid, pyramidize, sample_type,
					  pixel_type, num_bands, tile_w, tile_h,
					  compression, quality, stmt_data,
					  stmt_tils, stmt_sect, stmt_levl,
					  stmt_upd_sect);
		      sqlite3_free (path);
		      if (!ret)
			  goto error;
		      cnt++;
		  }
		if (_findnext (hFile, &c_file) != 0)
		    break;
	    };
	error:
	  _findclose (hFile);
      }
    sqlite3_free (search);
    return cnt;
#else
/* not Visual Studio .NET */
    int cnt = 0;
    char *path;
    struct dirent *entry;
    int ret;
    DIR *dir = opendir (dir_path);
    if (!dir)
	return 0;
    while (1)
      {
	  /* scanning dir-entries */
	  entry = readdir (dir);
	  if (!entry)
	      break;
	  if (!check_extension_match (entry->d_name, file_ext))
	      continue;
	  path = sqlite3_mprintf ("%s/%s", dir_path, entry->d_name);
	  ret =
	      do_import_file (handle, path, cvg, section, worldfile, force_srid,
			      pyramidize, sample_type, pixel_type, num_bands,
			      tile_w, tile_h, compression, quality, stmt_data,
			      stmt_tils, stmt_sect, stmt_levl, stmt_upd_sect);
	  sqlite3_free (path);
	  if (!ret)
	      goto error;
	  cnt++;
      }
  error:
    closedir (dir);
    return cnt;
#endif
}

static int
exec_import (sqlite3 * handle, const char *src_path, const char *dir_path,
	     const char *file_ext, const char *coverage, const char *section,
	     int worldfile, int force_srid, int pyramidize)
{
/* performing IMPORT */
    int ret;
    char *sql;
    rl2CoveragePtr cvg = NULL;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned short tile_w;
    unsigned short tile_h;
    unsigned char compression;
    int quality;
    char *table;
    char *xtable;
    unsigned short tileWidth;
    unsigned short tileHeight;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_upd_sect = NULL;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;

    tile_w = tileWidth;
    tile_h = tileHeight;
    rl2_get_coverage_compression (cvg, &compression, &quality);
    rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands);

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

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET statistics = ? WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_upd_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE sections SQL error: %s\n", sqlite3_errmsg (handle));
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

    if (dir_path == NULL)
      {
	  /* importing a single Image file */
	  if (!do_import_file
	      (handle, src_path, cvg, section, worldfile, force_srid,
	       pyramidize, sample_type, pixel_type, num_bands, tile_w, tile_h,
	       compression, quality, stmt_data, stmt_tils, stmt_sect,
	       stmt_levl, stmt_upd_sect))
	      goto error;
      }
    else
      {
	  /* importing all Image files from a whole directory */
	  if (!do_import_dir
	      (handle, dir_path, file_ext, cvg, section, worldfile, force_srid,
	       pyramidize, sample_type, pixel_type, num_bands, tile_w, tile_h,
	       compression, quality, stmt_data, stmt_tils, stmt_sect,
	       stmt_levl, stmt_upd_sect))
	      goto error;
      }

    sqlite3_finalize (stmt_upd_sect);
    sqlite3_finalize (stmt_sect);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);

    rl2_destroy_coverage (cvg);
    return 1;

  error:
    if (stmt_upd_sect != NULL)
	sqlite3_finalize (stmt_upd_sect);
    if (stmt_sect != NULL)
	sqlite3_finalize (stmt_sect);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return 0;
}

static void
copy_uint8_outbuf_to_tile (const unsigned char *outbuf, unsigned char *tile,
			   unsigned char num_bands, unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying UINT8 pixels from the output buffer into the tile */
    int x;
    int y;
    int b;
    const unsigned char *p_in;
    unsigned char *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in =
	      outbuf + ((base_y + y) * width * num_bands) +
	      (base_x * num_bands);
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out += num_bands;
		      p_in += num_bands;
		      continue;
		  }
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_from_outbuf_to_tile (const unsigned char *outbuf, unsigned char *tile,
			  unsigned char sample_type, unsigned char num_bands,
			  unsigned short width, unsigned short height,
			  unsigned short tile_width, unsigned short tile_height,
			  int base_y, int base_x)
{
/* copying pixels from the output buffer into the tile */
    switch (sample_type)
      {
      default:
	  copy_uint8_outbuf_to_tile ((unsigned char *) outbuf,
				     (unsigned char *) tile, num_bands,
				     width, height, tile_width, tile_height,
				     base_y, base_x);
	  break;
      };
}

static int
exec_export (sqlite3 * handle, const char *dst_path, const char *coverage,
	     double x_res, double y_res, double minx, double miny, double maxx,
	     double maxy, unsigned short width, unsigned short height)
{
/* performing EXPORT */
    rl2RasterPtr raster = NULL;
    rl2CoveragePtr cvg = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    int base_x;
    int base_y;

    if (rl2_find_matching_resolution
	(handle, coverage, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return 0;
    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;

    if (rl2_get_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, &outbuf, &outbuf_size, &palette) != RL2_OK)
	goto error;

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, pixel_type, num_bands,
					palette, RL2_COMPRESSION_NONE, 1, 256,
					srid, minx, miny, maxx, maxy, xx_res,
					yy_res, 0);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += 256)
      {
	  for (base_x = 0; base_x < width; base_x += 256)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * 256 * 256;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, 256, 256, palette);
		else
		    rl2_prime_void_tile (bufpix, 256, 256, sample_type,
					 num_bands);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, 256, 256,
					  base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (256, 256, sample_type, pixel_type,
				       num_bands, bufpix, bufpix_size, plt2,
				       NULL, 0, NULL);
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return 1;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 0;
}

static int
exec_drop (sqlite3 * handle, const char *coverage)
{
/* performing DROP */
    rl2CoveragePtr cvg = NULL;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (rl2_drop_dbms_coverage (handle, coverage) != RL2_OK)
	goto error;

    rl2_destroy_coverage (cvg);
    return 1;

  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return 0;
}

static int
exec_delete (sqlite3 * handle, const char *coverage, const char *section)
{
/* deleting a Raster Section */
    rl2CoveragePtr cvg = NULL;
    sqlite3_int64 section_id;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (rl2_get_dbms_section_id (handle, coverage, section, &section_id) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Section \"%s\" does not exists in Coverage \"%s\"\n",
		   section, coverage);
	  goto error;
      }

    if (rl2_delete_dbms_section (handle, coverage, section_id) != RL2_OK)
	goto error;

    rl2_destroy_coverage (cvg);
    return 1;

  error:
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
/* building a single Pyramid tile /
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
		/ retrieving lower-level tiles /
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
		      / scrolling the result set rows /
		      int ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE)
			  break;	/ end of result set /
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
			    fprintf (stderr, "SQL error: %s\n",
				     sqlite3_errmsg (handle));
			    return 0;
			}
		  }
		cx += params->tile_width * params->x_res;
		if (!done)
		  {
		      / lower-level tile not found - updating the Mask /
		      do_transparent_mask (params, col, row);
		  }
	    }
	  cy -= params->tile_height * params->y_res;
      }

/ saving the current Pyramid Tile /
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
/ INSERTing the tile /
    sqlite3_reset (stmt_tils);
    sqlite3_clear_bindings (stmt_tils);
    sqlite3_bind_int64 (stmt_tils, 1, params->section_id);
    /
       tile_maxx = tile_minx + ((double) tile_w * res_x);
       tile_miny = tile_maxy - ((double) tile_h * res_y);
     /
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
/ INSERTing tile data /
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
	*/
    return 0;
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
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int quality;
    unsigned short tileWidth;
    unsigned short tileHeight;
    int srid;
    sqlite3_stmt *stmt = NULL;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_compression (cvg, &compression, &quality) != RL2_OK)
	goto error;
    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;

    params.coverage = coverage;
    params.sample_type = sample_type;
    params.pixel_type = pixel_type;
    params.bands = num_bands;
    params.compression = compression;
    params.quality = quality;
    params.tile_width = tileWidth;
    params.tile_height = tileHeight;
    params.srid = srid;
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
	  if (rl2_get_dbms_section_id
	      (handle, coverage, section, &(params.section_id)) != RL2_OK)
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

static int
exec_catalog (sqlite3 * handle)
{
/* Rasterlite-2 datasources Catalog */
    const char *sql;
    int ret;
    int count = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *hres;
    char *vres;

    sql =
	"SELECT coverage_name, sample_type, pixel_type, num_bands, compression, "
	"quality, tile_width, tile_height, horz_resolution, vert_resolution, "
	"nodata_pixel, srid, auth_name, auth_srid, ref_sys_name "
	"FROM raster_coverages_ref_sys ORDER BY coverage_name";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	goto stop;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 0];
		const char *sample = results[(i * columns) + 1];
		const char *pixel = results[(i * columns) + 2];
		int bands = atoi (results[(i * columns) + 3]);
		const char *compression = results[(i * columns) + 4];
		int quality = atoi (results[(i * columns) + 5]);
		int tileW = atoi (results[(i * columns) + 6]);
		int tileH = atoi (results[(i * columns) + 7]);
		double x_res = atof (results[(i * columns) + 8]);
		double y_res = atof (results[(i * columns) + 9]);
		const char *nodata = "NONE";
		int srid = atoi (results[(i * columns) + 11]);
		const char *authName = results[(i * columns) + 12];
		int authSrid = atoi (results[(i * columns) + 13]);
		const char *crsName = results[(i * columns) + 14];
		count++;
		printf ("----------------------\n");
		printf ("             Coverage: %s\n", name);
		printf ("          Sample Type: %s\n", sample);
		printf ("           Pixel Type: %s\n", pixel);
		printf ("      Number of Bands: %d\n", bands);
		if (strcmp (compression, "NONE") == 0)
		    printf ("          Compression: NONE (uncompressed)\n");
		else if (strcmp (compression, "DEFLATE") == 0)
		    printf ("          Compression: DEFLATE (zip, lossless)\n");
		else if (strcmp (compression, "LZMA") == 0)
		    printf ("          Compression: LZMA (7-zip, lossless)\n");
		else if (strcmp (compression, "GIF") == 0)
		    printf ("          Compression: GIF, lossless\n");
		else if (strcmp (compression, "PNG") == 0)
		    printf ("          Compression: PNG, lossless\n");
		else if (strcmp (compression, "JPEG") == 0)
		    printf ("          Compression: JPEG (lossy)\n");
		else if (strcmp (compression, "LOSSY_WEBP") == 0)
		    printf ("          Compression: WEBP (lossy)\n");
		else if (strcmp (compression, "LOSSLESS_WEBP") == 0)
		    printf ("          Compression: WEBP, lossless\n");
		if (strcmp (compression, "JPEG") == 0
		    || strcmp (compression, "LOSSY_WEBP") == 0)
		    printf ("  Compression Quality: %d\n", quality);
		printf ("   Tile Size (pixels): %d x %d\n", tileW, tileH);
		printf ("                 Srid: %d (%s,%d) %s\n", srid,
			authName, authSrid, crsName);
		hres = formatFloat (x_res);
		vres = formatFloat (y_res);
		printf ("Pixel base resolution: X=%s Y=%s\n", hres, vres);
		sqlite3_free (hres);
		sqlite3_free (vres);
	    }
      }
    sqlite3_free_table (results);
    return 1;

  stop:
    printf ("no Rasterlite-2 datasources\n");
    return 0;
}

static int
exec_list (sqlite3 * handle, const char *coverage, const char *section)
{
/* Rasterlite-2 datasource Sections list */
    char *sql;
    int ret;
    int count = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *dumb1;
    char *dumb2;
    char *xsections;
    char *xxsections;
    char *xtiles;
    char *xxtiles;

    if (section == NULL)
      {
	  /* all sections */
	  xsections = sqlite3_mprintf ("%s_sections", coverage);
	  xxsections = gaiaDoubleQuotedSql (xsections);
	  sqlite3_free (xsections);
	  xtiles = sqlite3_mprintf ("%s_tiles", coverage);
	  xxtiles = gaiaDoubleQuotedSql (xtiles);
	  sqlite3_free (xtiles);
	  sql =
	      sqlite3_mprintf
	      ("SELECT s.section_id, s.section_name, s.width, s.height, "
	       "s.file_path, MbrMinX(s.geometry), MbrMinY(s.geometry), MbrMaxX(s.geometry), "
	       "MbrMaxY(s.geometry), Count(*) FROM \"%s\" AS s "
	       "JOIN \"%s\" AS t ON (s.section_id = t.section_id) "
	       "GROUP BY s.section_id", xxsections, xxtiles);
	  free (xxsections);
	  free (xxtiles);
      }
    else
      {
	  /* single section */
	  xsections = sqlite3_mprintf ("%s_sections", coverage);
	  xxsections = gaiaDoubleQuotedSql (xsections);
	  sqlite3_free (xsections);
	  xtiles = sqlite3_mprintf ("%s_tiles", coverage);
	  xxtiles = gaiaDoubleQuotedSql (xtiles);
	  sqlite3_free (xtiles);
	  sql =
	      sqlite3_mprintf
	      ("SELECT s.section_id, s.section_name, s.width, s.height, "
	       "s.file_path, MbrMinX(s.geometry), MbrMinY(s.geometry), MbrMaxX(s.geometry), "
	       "MbrMaxY(s.geometry), Count(*) FROM \"%s\" AS s "
	       "JOIN \"%s\" AS t ON (s.section_id = t.section_id) "
	       "WHERE s.section_name = %Q GROUP BY s.section_id", xxsections,
	       xxtiles, section);
	  free (xxsections);
	  free (xxtiles);
      }
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto stop;
    if (rows < 1)
	;
    else
      {
	  printf ("************ Coverage: %s\n", coverage);
	  for (i = 1; i <= rows; i++)
	    {
		int id = atoi (results[(i * columns) + 0]);
		const char *name = results[(i * columns) + 1];
		int width = atoi (results[(i * columns) + 2]);
		int height = atoi (results[(i * columns) + 3]);
		const char *path = results[(i * columns) + 4];
		double minx = atof (results[(i * columns) + 5]);
		double miny = atof (results[(i * columns) + 6]);
		double maxx = atof (results[(i * columns) + 7]);
		double maxy = atof (results[(i * columns) + 8]);
		int tiles = atoi (results[(i * columns) + 9]);
		count++;
		printf ("\nSection: %d) %s\n", id, name);
		printf ("    ------------------\n");
		printf ("        Size (pixels): %d x %d\n", width, height);
		printf ("           Input Path: %s\n", path);
		dumb1 = formatLong (minx);
		dumb2 = formatLat (miny);
		printf ("     LowerLeft corner: X=%s Y=%s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		dumb1 = formatLong (maxx);
		dumb2 = formatLat (maxy);
		printf ("    UpperRight corner: X=%s Y=%s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		dumb1 = formatLong (minx + ((maxx - minx) / 2.0));
		dumb2 = formatLat (miny + ((maxy - miny) / 2.0));
		printf ("         Center Point: X=%s Y=%s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		printf ("          Tiles Count: %d\n", tiles);
	    }
      }
    sqlite3_free_table (results);
    return 1;

  stop:
    printf ("not existing or empty Rasterlite-2 datasource\n");
    return 0;
}

static int
exec_map (sqlite3 * handle, const char *coverage, const char *dst_path,
	  unsigned short width, unsigned short height)
{
/* Rasterlite-2 datasource Map */
    char *sql;
    int ret;
    char *xsections;
    char *xxsections;
    sqlite3_stmt *stmt = NULL;
    int found = 0;
    double minx = 0.0;
    double maxx = 0.0;
    double miny = 0.0;
    double maxy = 0.0;
    double ext_x;
    double ext_y;
    double ratio_x;
    double ratio_y;
    double ratio;
    double cx;
    double cy;
    double base_x;
    double base_y;
    int row;
    int col;
    unsigned char *p_alpha;
    rl2GraphicsContextPtr ctx = NULL;
    rl2RasterPtr rst = NULL;
    rl2SectionPtr img = NULL;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    rl2GraphicsFontPtr font = NULL;

/* full extent */
    xsections = sqlite3_mprintf ("%s_sections", coverage);
    xxsections = gaiaDoubleQuotedSql (xsections);
    sqlite3_free (xsections);
    sql =
	sqlite3_mprintf
	("SELECT Min(MbrMinX(geometry)), Max(MbrMaxX(geometry)), "
	 "Min(MbrMinY(geometry)), Max(MbrMaxY(geometry)) "
	 "FROM \"%s\"", xxsections);
    free (xxsections);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT Coverage full Extent SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		minx = sqlite3_column_double (stmt, 0);
		maxx = sqlite3_column_double (stmt, 1);
		miny = sqlite3_column_double (stmt, 2);
		maxy = sqlite3_column_double (stmt, 3);
		found++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT Coverage full Extent; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (!found)
	goto error;

/* computing the reproduction ratio */
    ext_x = maxx - minx;
    ext_y = maxy - miny;
    ratio_x = (ext_x / 2.0) / (double) width;
    ratio_y = (ext_y / 2.0) / (double) height;
    if (ratio_x < ratio_y)
	ratio = ratio_x;
    else
	ratio = ratio_y;
    while (1)
      {
	  double w = ext_x / ratio;
	  double h = ext_y / ratio;
	  if (w < (double) (width - 20) && h < (double) (height - 20))
	      break;
	  ratio *= 1.001;
      }
    cx = minx + (ext_x / 2.0);
    cy = miny + (ext_y / 2.0);
    base_x = cx - ((double) (width / 2) * ratio);
    base_y = cy + ((double) (height / 2) * ratio);

/* creating a graphics context */
    ctx = rl2_graph_create_context (width, height);
    if (ctx == NULL)
      {
	  fprintf (stderr, "Unable to create a graphics backend\n");
	  goto error;
      }
/* setting up a black Font */
    font =
	rl2_graph_create_font (16, RL2_FONTSTYLE_ITALIC, RL2_FONTWEIGHT_BOLD);
    if (font == NULL)
      {
	  fprintf (stderr, "Unable to create a Font\n");
	  goto error;
      }
    if (!rl2_graph_font_set_color (font, 0, 0, 0, 255))
      {
	  fprintf (stderr, "Unable to set the font color\n");
	  goto error;
      }
    if (!rl2_graph_set_font (ctx, font))
      {
	  fprintf (stderr, "Unable to set up a font\n");
	  goto error;
      }

/* querying Sections */
    xsections = sqlite3_mprintf ("%s_sections", coverage);
    xxsections = gaiaDoubleQuotedSql (xsections);
    sqlite3_free (xsections);
    sql =
	sqlite3_mprintf
	("SELECT section_name, geometry " "FROM \"%s\"", xxsections);
    free (xxsections);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT Coverage full Extent SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *section_name =
		    (const char *) sqlite3_column_text (stmt, 0);
		const unsigned char *blob = sqlite3_column_blob (stmt, 1);
		int blob_sz = sqlite3_column_bytes (stmt, 1);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      double pre_x;
		      double pre_y;
		      double t_width;
		      double t_height;
		      double post_x;
		      double post_y;
		      double x = (geom->MinX - base_x) / ratio;
		      double y = (base_y - geom->MaxY) / ratio;
		      double w = (geom->MaxX - geom->MinX) / ratio;
		      double h = (geom->MaxY - geom->MinY) / ratio;
		      double cx = x + (w / 2.0);
		      double cy = y + (h / 2.0);
		      gaiaFreeGeomColl (geom);
		      /* setting up a RED pen */
		      rl2_graph_set_pen (ctx, 255, 0, 0, 255, 2.0,
					 RL2_PENSTYLE_SOLID);
		      /* setting up a Gray solid semi-transparent Brush */
		      rl2_graph_set_brush (ctx, 255, 255, 255, 204);
		      rl2_graph_draw_rectangle (ctx, x, y, w, h);
		      rl2_graph_get_text_extent (ctx, section_name, &pre_x,
						 &pre_y, &t_width, &t_height,
						 &post_x, &post_y);
		      /* setting up a white pen */
		      rl2_graph_set_pen (ctx, 255, 255, 255, 255, 2.0,
					 RL2_PENSTYLE_SOLID);
		      /* setting up a white solid semi-transparent Brush */
		      rl2_graph_set_brush (ctx, 255, 255, 255, 255);
		      rl2_graph_draw_rectangle (ctx, cx - (t_width / 2.0),
						cy - (t_height / 2.0),
						t_width, t_height);
		      rl2_graph_draw_text (ctx, section_name,
					   cx - (t_width / 2.0),
					   cy + (t_height / 2.0), 0.0);
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT Coverage full Extent; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);

/* exporting the Map as a PNG image */
    rgb = rl2_graph_get_context_rgb_array (ctx);
    if (rgb == NULL)
      {
	  fprintf (stderr, "invalid RGB buffer from Graphics Context\n");
	  goto error;
      }
    alpha = rl2_graph_get_context_alpha_array (ctx);
    if (alpha == NULL)
      {
	  fprintf (stderr, "invalid Alpha buffer from Graphics Context\n");
	  goto error;
      }
    rl2_graph_destroy_context (ctx);
    ctx = NULL;

/* transforming ALPHA into a transparency mask */
    p_alpha = alpha;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_alpha > 128)
		    *p_alpha++ = 1;
		else
		    *p_alpha++ = 0;
	    }
      }
    rst = rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     rgb, width * height * 3, NULL, alpha,
			     width * height, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to create the output raster+mask\n");
	  goto error;
      }
    rgb = NULL;
    alpha = NULL;
    img =
	rl2_create_section ("beta", RL2_COMPRESSION_NONE,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    rst);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create the output section+mask\n");
	  goto error;
      }
    if (rl2_section_to_png (img, dst_path) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: test_paint.png\n");
	  goto error;
      }
    rl2_destroy_section (img);
    rl2_graph_destroy_font (font);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    if (font != NULL)
	rl2_graph_destroy_font (font);
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    printf ("not existing or empty Rasterlite-2 datasource\n");
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
		   int pixel, int num_bands, int compression, int *quality,
		   int tile_width, int tile_height, int srid, double x_res,
		   double y_res)
{
/* checking/printing CREATE args */
    int err = 0;
    int res_error = 0;
    fprintf (stderr, "\n\nrl2_tool: request is CREATE\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "              DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "             Coverage: %s\n", coverage);
    switch (sample)
      {
      case RL2_SAMPLE_1_BIT:
	  fprintf (stderr, "          Sample Type: 1-BIT\n");
	  break;
      case RL2_SAMPLE_2_BIT:
	  fprintf (stderr, "          Sample Type: 2-BIT\n");
	  break;
      case RL2_SAMPLE_4_BIT:
	  fprintf (stderr, "          Sample Type: 4-BIT\n");
	  break;
      case RL2_SAMPLE_INT8:
	  fprintf (stderr, "          Sample Type: INT8\n");
	  break;
      case RL2_SAMPLE_UINT8:
	  fprintf (stderr, "          Sample Type: UINT8\n");
	  break;
      case RL2_SAMPLE_INT16:
	  fprintf (stderr, "          Sample Type: INT16\n");
	  break;
      case RL2_SAMPLE_UINT16:
	  fprintf (stderr, "          Sample Type: UINT16\n");
	  break;
      case RL2_SAMPLE_INT32:
	  fprintf (stderr, "          Sample Type: INT32\n");
	  break;
      case RL2_SAMPLE_UINT32:
	  fprintf (stderr, "          Sample Type: UINT32\n");
	  break;
      case RL2_SAMPLE_FLOAT:
	  fprintf (stderr, "          Sample Type: FLOAT\n");
	  break;
      case RL2_SAMPLE_DOUBLE:
	  fprintf (stderr, "          Sample Type: DOUBLE\n");
	  break;
      default:
	  fprintf (stderr, "*** ERROR *** unknown sample type\n");
	  err = 1;
	  break;
      };
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  fprintf (stderr, "           Pixel Type: MONOCHROME\n");
	  num_bands = 1;
	  break;
      case RL2_PIXEL_PALETTE:
	  fprintf (stderr, "           Pixel Type: PALETTE\n");
	  num_bands = 1;
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  fprintf (stderr, "           Pixel Type: GRAYSCALE\n");
	  num_bands = 1;
	  break;
      case RL2_PIXEL_RGB:
	  fprintf (stderr, "           Pixel Type: RGB\n");
	  num_bands = 3;
	  break;
      case RL2_PIXEL_MULTIBAND:
	  fprintf (stderr, "           Pixel Type: MULTIBAND\n");
	  break;
      case RL2_PIXEL_DATAGRID:
	  fprintf (stderr, "           Pixel Type: DATAGRID\n");
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
	fprintf (stderr, "      Number of Bands: %d\n", num_bands);
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  fprintf (stderr, "          Compression: NONE (uncompressed)\n");
	  *quality = 100;
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  fprintf (stderr, "          Compression: DEFLATE (zip, lossless)\n");
	  *quality = 100;
	  break;
      case RL2_COMPRESSION_LZMA:
	  fprintf (stderr, "          Compression: LZMA (7-zip, lossless)\n");
	  *quality = 100;
	  break;
      case RL2_COMPRESSION_GIF:
	  fprintf (stderr, "          Compression: GIF, lossless\n");
	  *quality = 100;
	  break;
      case RL2_COMPRESSION_PNG:
	  fprintf (stderr, "          Compression: PNG, lossless\n");
	  *quality = 100;
	  break;
      case RL2_COMPRESSION_JPEG:
	  fprintf (stderr, "          Compression: JPEG (lossy)\n");
	  if (*quality < 0)
	      *quality = 80;
	  if (*quality > 100)
	      *quality = 100;
	  fprintf (stderr, "  Compression Quality: %d\n", *quality);
	  break;
      case RL2_COMPRESSION_LOSSY_WEBP:
	  fprintf (stderr, "          Compression: WEBP (lossy)\n");
	  if (*quality < 0)
	      *quality = 80;
	  if (*quality > 100)
	      *quality = 100;
	  fprintf (stderr, "  Compression Quality: %d\n", *quality);
	  break;
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  fprintf (stderr, "          Compression: WEBP, lossless\n");
	  *quality = 100;
	  break;
      default:
	  fprintf (stderr, "*** ERROR *** unknown compression\n");
	  err = 1;
	  break;
      };
    fprintf (stderr, "   Tile size (pixels): %d x %d\n", tile_width,
	     tile_height);
    if (srid <= 0)
      {
	  fprintf (stderr, "*** ERROR *** undefined SRID\n");
	  err = 1;
      }
    else
	fprintf (stderr, "                 Srid: %d\n", srid);
    if (x_res == DBL_MAX || y_res <= 0.0)
      {
	  fprintf (stderr, "*** ERROR *** invalid X pixel size\n");
	  err = 1;
	  res_error = 1;
      }
    if (y_res == DBL_MAX || y_res <= 0.0)
      {
	  fprintf (stderr, "*** ERROR *** invalid Y pixel size\n");
	  err = 1;
	  res_error = 1;
      }
    if (x_res > 0.0 && y_res > 0.0 && !res_error)
      {
	  char *hres = formatFloat (x_res);
	  char *vres = formatFloat (y_res);
	  fprintf (stderr, "Pixel base resolution: X=%s Y=%s\n", hres, vres);
	  sqlite3_free (hres);
	  sqlite3_free (vres);
      }
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_import_args (const char *db_path, const char *src_path,
		   const char *dir_path, const char *file_ext,
		   const char *coverage, const char *section, int worldfile,
		   int srid, int pyramidize)
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
	fprintf (stderr, "              DB path: %s\n", db_path);

    if (src_path != NULL && dir_path == NULL)
	fprintf (stderr, "    Input Source path: %s\n", src_path);
    else if (src_path == NULL && dir_path != NULL && file_ext != NULL)
      {
	  fprintf (stderr, " Input Directory path: %s\n", dir_path);
	  fprintf (stderr, "       File Extension: %s\n", file_ext);
      }
    else
      {
	  if (src_path == NULL && dir_path == NULL)
	    {
		fprintf (stderr,
			 "*** ERROR *** no input Source path was specified\n");
		fprintf (stderr,
			 "*** ERROR *** no input Directory path was specified\n");
		err = 1;
	    }
	  if (dir_path != NULL && src_path != NULL)
	    {
		fprintf (stderr,
			 "*** ERROR *** both input Source and Directory were specified (mutually exclusive)\n");
		err = 1;
	    }
	  if (dir_path != NULL && file_ext == NULL)
	    {
		fprintf (stderr, "*** ERROR *** no File Extension specified\n");
		err = 1;
	    }
      }

    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "             Coverage: %s\n", coverage);
    if (section == NULL)
	fprintf (stderr, "              Section: from file name\n");
    else
	fprintf (stderr, "              Section: %s\n", section);
    if (worldfile)
      {
	  if (srid <= 0)
	      fprintf (stderr,
		       "*** ERROR *** WorldFile requires specifying some SRID\n");
	  else
	      fprintf (stderr, "Using the WorldFile\n");
      }
    if (srid > 0)
	fprintf (stderr, "          Forced SRID: %d\n", srid);
    if (pyramidize)
	fprintf (stderr, "Immediately building Pyramid Levels\n");
    else
	fprintf (stderr, "Ignoring Pyramid Levels for now\n");
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_export_args (const char *db_path, const char *dst_path,
		   const char *coverage, double x_res, double y_res,
		   double *minx, double *miny, double *maxx, double *maxy,
		   double *cx, double *cy, unsigned short *width,
		   unsigned short *height)
{
/* checking/printing EXPORT args */
    double ext_x;
    double ext_y;
    char *dumb1;
    char *dumb2;
    int err = 0;
    int err_bbox = 0;
    fprintf (stderr, "\n\nrl2_tool; request is EXPORT\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "           DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "          Coverage: %s\n", coverage);
    if (x_res == DBL_MAX || y_res <= 0.0)
      {
	  fprintf (stderr, "*** ERROR *** invalid X pixel size\n");
	  err = 1;
      }
    if (y_res == DBL_MAX || y_res <= 0.0)
      {
	  fprintf (stderr, "*** ERROR *** invalid Y pixel size\n");
	  err = 1;
      }
    if (x_res > 0.0 && y_res > 0.0)
      {
	  dumb1 = formatFloat (x_res);
	  dumb2 = formatFloat (y_res);
	  fprintf (stderr, "        Pixel size: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    if (*cx == DBL_MAX && *cy == DBL_MAX && *width == 0 && *height == 0)
      {
	  /* computing the image center, width and height */
	  if (*minx == DBL_MAX)
	    {
		fprintf (stderr,
			 "*** ERROR *** undeclared Min-X (lower-left corner)\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*maxx == DBL_MAX)
	    {
		fprintf (stderr,
			 "*** ERROR *** undeclared Max-X (upper-right corner)\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*miny == DBL_MAX)
	    {
		fprintf (stderr,
			 "*** ERROR *** undeclared Min-Y (lower-left corner)\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*maxy == DBL_MAX)
	    {
		fprintf (stderr,
			 "*** ERROR *** undeclared Max-Y (upper-right corner)\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*maxx <= *minx)
	    {
		fprintf (stderr,
			 "*** ERROR *** negative or NULL horizontal extent\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*maxy <= *miny)
	    {
		fprintf (stderr,
			 "*** ERROR *** negative or NULL vertical extent\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (!err_bbox)
	    {
		dumb1 = formatLong (*minx);
		dumb2 = formatLat (*miny);
		fprintf (stderr, " Lower-Left Corner: %s %s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		dumb1 = formatLong (*maxx);
		dumb2 = formatLat (*maxy);
		fprintf (stderr, "Upper-Right Corner: %s %s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		ext_x = *maxx - *minx;
		ext_y = *maxy - *miny;
		*cx = *minx + (ext_x / 2.0);
		*cy = *miny + (ext_y / 2.0);
		dumb1 = formatLong (*cx);
		dumb2 = formatLat (*cy);
		fprintf (stderr, "            Center: %s %s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		if ((ext_x / x_res) > USHRT_MAX)
		  {
		      fprintf (stderr,
			       "*** ERROR *** exceeding max image Width\n");
		      err = 1;
		      err_bbox = 1;
		  }
		else
		    *width = (unsigned short) (ext_x / x_res);
		if ((ext_y / y_res) > USHRT_MAX)
		  {
		      fprintf (stderr,
			       "*** ERROR *** exceeding max image Height\n");
		      err = 1;
		      err_bbox = 1;
		  }
		else
		    *height = (unsigned short) (ext_y / y_res);
		if (!err_bbox)
		    fprintf (stderr, "        Image Size: %u x %u\n", *width,
			     *height);
	    }
      }
    else if (*minx == DBL_MAX && *miny == DBL_MAX && *maxx == DBL_MAX
	     && *maxy == DBL_MAX)
      {
	  if (*cx == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Center-X\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*cy == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Center-Y\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*width == 0)
	    {
		fprintf (stderr, "*** ERROR *** NULL/ZERO image Width\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*height == 0)
	    {
		fprintf (stderr, "*** ERROR *** NULL/ZERO image Height\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (!err_bbox)
	    {
		ext_x = (double) (*width) * x_res;
		ext_y = (double) (*height) * y_res;
		*minx = *cx - (ext_x / 2.0);
		*maxx = *minx + ext_x;
		*miny = *cy - (ext_y / 2.0);
		*maxy = *miny + ext_y;
		dumb1 = formatLong (*minx);
		dumb2 = formatLat (*miny);
		fprintf (stderr, " Lower-Left Corner: %s %s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		dumb1 = formatLong (*maxx);
		dumb2 = formatLat (*maxy);
		fprintf (stderr, "Upper-Right Corner: %s %s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		dumb1 = formatLong (*cx);
		dumb2 = formatLat (*cy);
		fprintf (stderr, "            Center: %s %s\n", dumb1, dumb2);
		sqlite3_free (dumb1);
		sqlite3_free (dumb2);
		fprintf (stderr, "        Image Size: %u x %u\n", *width,
			 *height);
	    }
	  if (dst_path == NULL)
	    {
		fprintf (stderr,
			 "*** ERROR *** no output Destination path was specified\n");
		err = 1;
	    }
	  else
	      fprintf (stderr, "  Destination Path: %s\n", dst_path);
      }
    else
      {
	  fprintf (stderr,
		   "*** ERROR *** unable to determine the BBOX and image size\n");
	  err = 1;
      }
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
	fprintf (stderr, " DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "Coverage: %s\n", coverage);
    if (section == NULL)
	fprintf (stderr, " Section: All Sections\n");
    else
	fprintf (stderr, " Section: %s\n", section);
    fprintf (stderr,
	     "===========================================================\n\n");
    return err;
}

static int
check_map_args (const char *db_path, const char *coverage, const char *dst_path,
		unsigned short *width, unsigned short *height)
{
/* checking/printing MAP args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is MAP\n");
    fprintf (stderr,
	     "===========================================================\n");
    if (db_path == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no DB path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "           DB path: %s\n", db_path);
    if (coverage == NULL)
      {
	  fprintf (stderr, "*** ERROR *** no Coverage's name was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "          Coverage: %s\n", coverage);
    if (dst_path == NULL)
      {
	  fprintf (stderr,
		   "*** ERROR *** no output Destination path was specified\n");
	  err = 1;
      }
    else
	fprintf (stderr, "  Destination Path: %s\n", dst_path);
    if (*width < 512)
	*width = 512;
    if (*width > 4092)
	*width = 4092;
    if (*height < 512)
	*height = 512;
    if (*height > 4096)
	*height = 4096;
    fprintf (stderr, "        Image Size: %u x %u\n", *width, *height);
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
do_help (int mode)
{
/* printing the argument list */
    fprintf (stderr, "\n\nusage: rl2_tool MODE [ ARGLIST ]\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr,
	     "-h or --help                    print this help message\n");
    if (mode == ARG_NONE || mode == ARG_MODE_CREATE)
      {
	  /* MODE = CREATE */
	  fprintf (stderr, "\nmode: CREATE\n");
	  fprintf (stderr, "will create a new RasterLite2 Raster Coverage\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
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
	  fprintf (stderr,
		   "-tlw or --tile-width  integer   Tile Width [pixels]\n");
	  fprintf (stderr,
		   "-tlh or --tile-height integer   Tile Height [pixels]\n");
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
	  fprintf (stderr,
		   "MONOCHROME PALETTE GRAYSCALE RGB MULTIBAND DATAGRID\n\n");
	  fprintf (stderr, "Compression Keywords:\n");
	  fprintf (stderr, "----------------------------------\n");
	  fprintf (stderr,
		   "NONE DEFLATE LZMA GIF PNG JPEG LOSSY_WEBP LOSSLESS_WEBP\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_DROP)
      {
	  /* MODE = DROP */
	  fprintf (stderr, "\nmode: DROP\n");
	  fprintf (stderr,
		   "will drop an existing RasterLite2 Raster Coverage\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr,
		   "-cov or --coverage    string    Coverage's name\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_IMPORT)
      {
	  /* MODE = IMPORT */
	  fprintf (stderr, "\nmode: IMPORT\n");
	  fprintf (stderr,
		   "will create a new Raster Section by importing an\n");
	  fprintf (stderr, "external image or raster file\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr,
		   "-src or --src-path    pathname  input Image/Raster path\n");
	  fprintf (stderr,
		   "-dir or --dir-path    pathname  input directory path\n");
	  fprintf (stderr,
		   "-ext or --file-ext    extension file extension (e.g. .tif)\n");
	  fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
	  fprintf (stderr,
		   "-sec or --section     string    optional: Section's name\n");
	  fprintf (stderr,
		   "-srid or --srid       integer   optional: force SRID value\n");
	  fprintf (stderr,
		   "-wf or --worldfile              requires a Worldfile\n");
	  fprintf (stderr,
		   "-pyr or --pyramidize            immediately build Pyramid levels\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_EXPORT)
      {
	  /* MODE = EXPORT */
	  fprintf (stderr, "\nmode: EXPORT\n");
	  fprintf (stderr, "will export an external image from a Coverage\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr,
		   "-dst or --dst-path    pathname  output Image/Raster path\n");
	  fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
	  fprintf (stderr,
		   "-res or --resolution  number    pixel resolution(X and Y)\n");
	  fprintf (stderr,
		   "-xres or --x-resol    number    pixel resolution(X specific)\n");
	  fprintf (stderr,
		   "-yres or --y-resol    number    pixel resolution(Y specific)\n");
	  fprintf (stderr,
		   "-minx or --min-x      number    X coordinate (lower-left corner)\n");
	  fprintf (stderr,
		   "-miny or --min-y      number    Y coordinate (lower-left corner)\n");
	  fprintf (stderr,
		   "-maxx or --max-x      number    X coordinate (upper-right corner)\n");
	  fprintf (stderr,
		   "-maxy or --max-y      number    Y coordinate (upper-left corner)\n");
	  fprintf (stderr,
		   "-cx or --center-x     number    X coordinate (center)\n");
	  fprintf (stderr,
		   "-cy or --center-y     number    Y coordinate (center)\n");
	  fprintf (stderr,
		   "-outw or --out-width  number    image width (in pixels)\n");
	  fprintf (stderr,
		   "-outh or --out-height number    image height (in pixels)\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_DELETE)
      {
	  /* MODE = DELETE */
	  fprintf (stderr, "\nmode: DELETE\n");
	  fprintf (stderr, "will delete a Raster Section\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
	  fprintf (stderr,
		   "-sec or --section     string    Section's name\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_PYRAMIDIZE)
      {
	  /* MODE = PYRAMIDIZE */
	  fprintf (stderr, "\nmode: PYRAMIDIZE\n");
	  fprintf (stderr,
		   "will (re)build all Pyramid levels supporting a Coverage\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
	  fprintf (stderr,
		   "-sec or --section     string    optional: Section's name\n");
	  fprintf (stderr,
		   "                                default is \"All Sections\"\n");
	  fprintf (stderr,
		   "-f or --force                   optional: rebuilds from scratch\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_LIST)
      {
	  /* MODE = LIST */
	  fprintf (stderr, "\nmode: LIST\n");
	  fprintf (stderr, "will list Raster Sections within a Coverage\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
	  fprintf (stderr,
		   "-sec or --section     string    optional: Section's name\n");
	  fprintf (stderr,
		   "                                default is \"All Sections\"\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_MAP)
      {
	  /* MODE = MAP */
	  fprintf (stderr, "\nmode: MAP\n");
	  fprintf (stderr,
		   "will output a PNG Map representing all Raster Sections\nwithin a Coverage\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr, "-cov or --coverage    string    Coverage's name\n");
	  fprintf (stderr,
		   "-dst or --dst-path    pathname  output Image/Raster path\n");
	  fprintf (stderr,
		   "-outw or --out-width  number    image width (in pixels)\n");
	  fprintf (stderr,
		   "-outh or --out-height number    image height (in pixels)\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_CATALOG)
      {
	  /* MODE = CATALOG */
	  fprintf (stderr, "\nmode: CATALOG\n");
	  fprintf (stderr,
		   "will list all Coverages from within a RasterLite2 DB\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n\n");
      }
    if (mode == ARG_NONE || mode == ARG_MODE_CHECK)
      {
	  /* MODE = CHECK */
	  fprintf (stderr, "\nmode: CHECK\n");
	  fprintf (stderr, "will check a RasterLite2 DB for validity\n");
	  fprintf (stderr,
		   "==============================================================\n");
	  fprintf (stderr,
		   "-db or --db-path      pathname  RasterLite2 DB path\n");
	  fprintf (stderr,
		   "-cov or --coverage    string    optional: Coverage's name\n");
	  fprintf (stderr,
		   "                                default is \"All Coverages\"\n");
	  fprintf (stderr,
		   "-sec or --section     string    optional: Section's name\n");
	  fprintf (stderr,
		   "                                default is \"All Sections\"\n\n");
      }
    if (mode == ARG_NONE)
      {
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
    const char *dst_path = NULL;
    const char *dir_path = NULL;
    const char *file_ext = NULL;
    const char *coverage = NULL;
    const char *section = NULL;
    unsigned char sample = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel = RL2_PIXEL_UNKNOWN;
    unsigned char compression = RL2_COMPRESSION_NONE;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    int quality = -1;
    unsigned short tile_width = 512;
    unsigned short tile_height = 512;
    double x_res = DBL_MAX;
    double y_res = DBL_MAX;
    double minx = DBL_MAX;
    double miny = DBL_MAX;
    double maxx = DBL_MAX;
    double maxy = DBL_MAX;
    double cx = DBL_MAX;
    double cy = DBL_MAX;
    unsigned short width = 0;
    unsigned short height = 0;
    int srid = -1;
    int worldfile = 0;
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
	  if (strcasecmp (argv[1], "EXPORT") == 0)
	      mode = ARG_MODE_EXPORT;
	  if (strcasecmp (argv[1], "DELETE") == 0)
	      mode = ARG_MODE_DELETE;
	  if (strcasecmp (argv[1], "PYRAMIDIZE") == 0)
	      mode = ARG_MODE_PYRAMIDIZE;
	  if (strcasecmp (argv[1], "LIST") == 0)
	      mode = ARG_MODE_LIST;
	  if (strcasecmp (argv[1], "MAP") == 0)
	      mode = ARG_MODE_MAP;
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
		  case ARG_DST_PATH:
		      dst_path = argv[i];
		      break;
		  case ARG_DIR_PATH:
		      dir_path = argv[i];
		      break;
		  case ARG_FILE_EXT:
		      file_ext = argv[i];
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
		  case ARG_MINX:
		      minx = atof (argv[i]);
		      break;
		  case ARG_MINY:
		      miny = atof (argv[i]);
		      break;
		  case ARG_MAXX:
		      maxx = atof (argv[i]);
		      break;
		  case ARG_MAXY:
		      maxy = atof (argv[i]);
		      break;
		  case ARG_CX:
		      cx = atof (argv[i]);
		      break;
		  case ARG_CY:
		      cy = atof (argv[i]);
		      break;
		  case ARG_IMG_WIDTH:
		      width = atoi (argv[i]);
		      break;
		  case ARG_IMG_HEIGHT:
		      height = atoi (argv[i]);
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
		do_help (mode);
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
	  if (strcmp (argv[i], "-dst") == 0
	      || strcasecmp (argv[i], "--dst-path") == 0)
	    {
		next_arg = ARG_DST_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-dir") == 0
	      || strcasecmp (argv[i], "--dir-path") == 0)
	    {
		next_arg = ARG_DIR_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-ext") == 0
	      || strcasecmp (argv[i], "--file-ext") == 0)
	    {
		next_arg = ARG_FILE_EXT;
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
	  if (strcmp (argv[i], "-outw") == 0
	      || strcasecmp (argv[i], "--out-width") == 0)
	    {
		next_arg = ARG_IMG_WIDTH;
		continue;
	    }
	  if (strcmp (argv[i], "-outh") == 0
	      || strcasecmp (argv[i], "--out-height") == 0)
	    {
		next_arg = ARG_IMG_HEIGHT;
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
	  if (strcmp (argv[i], "-minx") == 0
	      || strcasecmp (argv[i], "--min-x") == 0)
	    {
		next_arg = ARG_MINX;
		continue;
	    }
	  if (strcmp (argv[i], "-miny") == 0
	      || strcasecmp (argv[i], "--min-y") == 0)
	    {
		next_arg = ARG_MINY;
		continue;
	    }
	  if (strcmp (argv[i], "-maxx") == 0
	      || strcasecmp (argv[i], "--max-x") == 0)
	    {
		next_arg = ARG_MAXX;
		continue;
	    }
	  if (strcmp (argv[i], "-maxy") == 0
	      || strcasecmp (argv[i], "--max-y") == 0)
	    {
		next_arg = ARG_MAXY;
		continue;
	    }
	  if (strcmp (argv[i], "-cx") == 0
	      || strcasecmp (argv[i], "--center-x") == 0)
	    {
		next_arg = ARG_CX;
		continue;
	    }
	  if (strcmp (argv[i], "-cy") == 0
	      || strcasecmp (argv[i], "--center-y") == 0)
	    {
		next_arg = ARG_CY;
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
	  if (strcmp (argv[i], "-wf") == 0
	      || strcasecmp (argv[i], "--worldfile") == 0)
	    {
		worldfile = 1;
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
	  do_help (mode);
	  return -1;
      }

/* checking the arguments */
    switch (mode)
      {
      case ARG_MODE_CREATE:
	  error =
	      check_create_args (db_path, coverage, sample, pixel, num_bands,
				 compression, &quality, tile_width, tile_height,
				 srid, x_res, y_res);
	  break;
      case ARG_MODE_DROP:
	  error = check_drop_args (db_path, coverage);
	  break;
      case ARG_MODE_IMPORT:
	  error =
	      check_import_args (db_path, src_path, dir_path, file_ext,
				 coverage, section, worldfile, srid,
				 pyramidize);
	  break;
      case ARG_MODE_EXPORT:
	  error =
	      check_export_args (db_path, dst_path, coverage, x_res, y_res,
				 &minx, &miny, &maxx, &maxy, &cx, &cy, &width,
				 &height);
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
      case ARG_MODE_MAP:
	  error = check_map_args (db_path, coverage, dst_path, &width, &height);
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
	  do_help (mode);
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
	      exec_import (handle, src_path, dir_path, file_ext, coverage,
			   section, worldfile, srid, pyramidize);
	  break;
      case ARG_MODE_EXPORT:
	  ret =
	      exec_export (handle, dst_path, coverage, x_res, y_res, minx, miny,
			   maxx, maxy, width, height);
	  break;
      case ARG_MODE_DELETE:
	  ret = exec_delete (handle, coverage, section);
	  break;
      case ARG_MODE_PYRAMIDIZE:
	  ret = exec_pyramidize (handle, coverage, section, force_pyramid);
	  break;
      case ARG_MODE_CATALOG:
	  ret = exec_catalog (handle);
	  break;
      case ARG_MODE_LIST:
	  ret = exec_list (handle, coverage, section);
	  break;
      case ARG_MODE_MAP:
	  ret = exec_map (handle, coverage, dst_path, width, height);
	  break;
/*
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
	    case ARG_MODE_EXPORT:
		op_name = "EXPORT";
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
	    case ARG_MODE_MAP:
		op_name = "MAP";
		break;
	    case ARG_MODE_CATALOG:
		op_name = "CATALOG";
		break;
	    case ARG_MODE_CHECK:
		op_name = "CHECK";
		break;
	    };
	  fprintf (stderr, "\nOperation %s successfully completed\n", op_name);
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
	  printf ("\tIN_MEMORY database successfully exported\n");
      }

  stop:
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();
    return 0;
}
