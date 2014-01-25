/*

 rl2import -- DBMS import functions

 version 0.1, 2014 January 9

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

#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__)
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"
#include "rasterlite2_private.h"

#include <spatialite/gaiaaux.h>

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
    int xpos2;
    int len;
    char *name;
    const char *p;
    if (src_path == NULL)
	return NULL;
    pos2 = strlen (src_path) - 1;
    xpos2 = pos2;
    pos1 = 0;
    p = src_path + pos2;
    while (p >= src_path)
      {
	  if (*p == '.' && pos2 == xpos2)
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
do_insert_section (sqlite3 * handle, const char *src_path, const char *section,
		   int srid, unsigned short width, unsigned short height,
		   double minx, double miny, double maxx, double maxy,
		   sqlite3_stmt * stmt_sect, sqlite3_int64 * id)
{
/* INSERTing the section */
    int ret;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    sqlite3_int64 section_id;

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
    *id = section_id;
    return 1;
  error:
    return 0;
}

static int
do_insert_levels (sqlite3 * handle, double res_x, double res_y,
		  sqlite3_stmt * stmt_levl)
{
/* INSERTing the base-levels */
    int ret;
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
    return 1;
  error:
    return 0;
}

static int
do_insert_tile (sqlite3 * handle, unsigned char *blob_odd, int blob_odd_sz,
		unsigned char *blob_even, int blob_even_sz,
		sqlite3_int64 section_id, int srid, double res_x, double res_y,
		unsigned short tile_w, unsigned short tile_h, double minx,
		double miny, double maxx, double maxy, double *tile_minx,
		double *tile_miny, double *tile_maxx, double *tile_maxy,
		rl2PalettePtr aux_palette, rl2PixelPtr no_data,
		sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data,
		rl2RasterStatisticsPtr section_stats)
{
/* INSERTing the tile */
    int ret;
    sqlite3_int64 tile_id;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    rl2RasterStatisticsPtr stats = NULL;

    stats = rl2_get_raster_statistics
	(blob_odd, blob_odd_sz, blob_even, blob_even_sz, aux_palette, no_data);
    if (stats == NULL)
	goto error;
    rl2_aggregate_raster_statistics (stats, section_stats);
    sqlite3_reset (stmt_tils);
    sqlite3_clear_bindings (stmt_tils);
    sqlite3_bind_int64 (stmt_tils, 1, section_id);
    *tile_maxx = *tile_minx + ((double) tile_w * res_x);
    if (*tile_maxx > maxx)
	*tile_maxx = maxx;
    *tile_miny = *tile_maxy - ((double) tile_h * res_y);
    if (*tile_miny < miny)
	*tile_miny = miny;
    geom = build_extent (srid, *tile_minx, *tile_miny, *tile_maxx, *tile_maxy);
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
	sqlite3_bind_blob (stmt_data, 3, blob_even, blob_even_sz, free);
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
    rl2_destroy_raster_statistics (stats);
    return 1;
  error:
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    return 0;
}

static int
do_insert_stats (sqlite3 * handle, rl2RasterStatisticsPtr section_stats,
		 sqlite3_int64 section_id, sqlite3_stmt * stmt_upd_sect)
{
/* updating the Section's Statistics */
    unsigned char *blob_stats;
    int blob_stats_sz;
    int ret;

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
    return 1;
  error:
    return 0;
}

static int
do_import_ascii_grid (sqlite3 * handle, const char *src_path,
		      rl2CoveragePtr cvg, const char *section, int srid,
		      unsigned short tile_w, unsigned short tile_h,
		      int pyramidize, unsigned char sample_type,
		      unsigned char compression, sqlite3_stmt * stmt_data,
		      sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
		      sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect)
{
/* importing an ASCII Data Grid file */
    int ret;
    rl2AsciiGridOriginPtr origin = NULL;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    int row;
    int col;
    unsigned short width;
    unsigned short height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
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
    sqlite3_int64 section_id;

    origin = rl2_create_ascii_grid_origin (src_path, srid, sample_type);
    if (origin == NULL)
	goto error;

    printf ("Importing: %s\n", rl2_get_ascii_grid_origin_path (origin));
    printf ("------------------\n");
    ret = rl2_get_ascii_grid_origin_size (origin, &width, &height);
    if (ret == RL2_OK)
	printf ("    Image Size (pixels): %d x %d\n", width, height);
    ret = rl2_get_ascii_grid_origin_srid (origin, &srid);
    if (ret == RL2_OK)
	printf ("                   SRID: %d\n", srid);
    ret = rl2_get_ascii_grid_origin_extent (origin, &minx, &miny, &maxx, &maxy);
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
    ret = rl2_get_ascii_grid_origin_resolution (origin, &res_x, &res_y);
    if (ret == RL2_OK)
      {
	  dumb1 = formatFloat (res_x);
	  dumb2 = formatFloat (res_y);
	  printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }

    if (rl2_eval_ascii_grid_origin_compatibility (cvg, origin) != RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/ASCII mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, 1);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (!do_insert_levels (handle, res_x, res_y, stmt_levl))
	goto error;

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_ascii_grid_origin (cvg, origin, row, col);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "ERROR: unable to get a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }
		if (rl2_raster_encode
		    (raster, compression, &blob_odd, &blob_odd_sz, &blob_even,
		     &blob_even_sz, 100, 1) != RL2_OK)
		  {
		      fprintf (stderr,
			       "ERROR: unable to encode a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }

		/* INSERTing the tile */
		if (!do_insert_tile
		    (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz,
		     section_id, srid, res_x, res_y, tile_w, tile_h, minx, miny,
		     maxx, maxy, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

/* updating the Section's Statistics */
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_ascii_grid_origin (origin);
    rl2_destroy_raster_statistics (section_stats);

    return 1;

  error:
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    if (origin != NULL)
	rl2_destroy_ascii_grid_origin (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

static int
is_ascii_grid (const char *path)
{
/* testing for an ASCII Grid */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".asc") == 0)
	      return 1;
      }
    return 0;
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
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    int row;
    int col;
    unsigned short width;
    unsigned short height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
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
    sqlite3_int64 section_id;

    if (is_ascii_grid (src_path))
	return do_import_ascii_grid (handle, src_path, cvg, section, force_srid,
				     tile_w, tile_h, pyramidize, sample_type,
				     compression, stmt_data, stmt_tils,
				     stmt_sect, stmt_levl, stmt_upd_sect);

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
      {
	  if (force_srid > 0 && force_srid != srid)
	    {
		printf ("                   SRID: %d (forced to %d)\n", srid,
			force_srid);
		srid = force_srid;
	    }
	  else
	      printf ("                   SRID: %d\n", srid);
      }
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

    if (rl2_eval_tiff_origin_compatibility (cvg, origin, force_srid) !=
	RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (!do_insert_levels (handle, res_x, res_y, stmt_levl))
	goto error;

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_tiff_origin (cvg, origin, row, col, srid);
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

		if (!do_insert_tile
		    (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz,
		     section_id, srid, res_x, res_y, tile_w, tile_h, minx, miny,
		     maxx, maxy, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     aux_palette, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

/* updating the Section's Statistics */
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

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
    if (strcasecmp (mark, file_ext) == 0)
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
do_import_common (sqlite3 * handle, const char *src_path, const char *dir_path,
		  const char *file_ext, rl2CoveragePtr cvg, const char *section,
		  int worldfile, int force_srid, int pyramidize)
{
/* main IMPORT Raster function */
    int ret;
    char *sql;
    const char *coverage;
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

    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;

    tile_w = tileWidth;
    tile_h = tileHeight;
    rl2_get_coverage_compression (cvg, &compression, &quality);
    rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands);
    coverage = rl2_get_coverage_name (cvg);

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
    stmt_upd_sect = NULL;
    stmt_sect = NULL;
    stmt_levl = NULL;
    stmt_tils = NULL;
    stmt_data = NULL;

    if (rl2_update_dbms_coverage (handle, coverage) != RL2_OK)
      {
	  fprintf (stderr, "unable to update the Coverage\n");
	  goto error;
      }

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
    return 0;
}

RL2_DECLARE int
rl2_load_raster_into_dbms (sqlite3 * handle, const char *src_path,
			   rl2CoveragePtr coverage, int worldfile,
			   int force_srid, int pyramidize)
{
/* importing a single Raster file */
    if (!do_import_common
	(handle, src_path, NULL, NULL, coverage, NULL, worldfile, force_srid,
	 pyramidize))
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_load_mrasters_into_dbms (sqlite3 * handle, const char *dir_path,
			     const char *file_ext,
			     rl2CoveragePtr coverage, int worldfile,
			     int force_srid, int pyramidize)
{
/* importing multiple Raster files from dir */
    if (!do_import_common
	(handle, NULL, dir_path, file_ext, coverage, NULL, worldfile,
	 force_srid, pyramidize))
	return RL2_ERROR;
    return RL2_OK;
}

static void
copy_int8_outbuf_to_tile (const char *outbuf, char *tile,
			  unsigned short width,
			  unsigned short height,
			  unsigned short tile_width,
			  unsigned short tile_height, int base_y, int base_x)
{
/* copying INT8 pixels from the output buffer into the tile */
    int x;
    int y;
    const char *p_in;
    char *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
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
copy_int16_outbuf_to_tile (const short *outbuf, short *tile,
			   unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying INT16 pixels from the output buffer into the tile */
    int x;
    int y;
    const short *p_in;
    short *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint16_outbuf_to_tile (const unsigned short *outbuf, unsigned short *tile,
			    unsigned char num_bands, unsigned short width,
			    unsigned short height,
			    unsigned short tile_width,
			    unsigned short tile_height, int base_y, int base_x)
{
/* copying UINT16 pixels from the output buffer into the tile */
    int x;
    int y;
    int b;
    const unsigned short *p_in;
    unsigned short *p_out = tile;

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
copy_int32_outbuf_to_tile (const int *outbuf, int *tile,
			   unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying INT32 pixels from the output buffer into the tile */
    int x;
    int y;
    const int *p_in;
    int *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint32_outbuf_to_tile (const unsigned int *outbuf, unsigned int *tile,
			    unsigned short width,
			    unsigned short height,
			    unsigned short tile_width,
			    unsigned short tile_height, int base_y, int base_x)
{
/* copying UINT32 pixels from the output buffer into the tile */
    int x;
    int y;
    const unsigned int *p_in;
    unsigned int *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_float_outbuf_to_tile (const float *outbuf, float *tile,
			   unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying FLOAT pixels from the output buffer into the tile */
    int x;
    int y;
    const float *p_in;
    float *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_double_outbuf_to_tile (const double *outbuf, double *tile,
			    unsigned short width,
			    unsigned short height,
			    unsigned short tile_width,
			    unsigned short tile_height, int base_y, int base_x)
{
/* copying DOUBLE pixels from the output buffer into the tile */
    int x;
    int y;
    const double *p_in;
    double *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
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
      case RL2_SAMPLE_INT8:
	  copy_int8_outbuf_to_tile ((char *) outbuf,
				    (char *) tile, width, height, tile_width,
				    tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_INT16:
	  copy_int16_outbuf_to_tile ((short *) outbuf,
				     (short *) tile, width, height, tile_width,
				     tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_outbuf_to_tile ((unsigned short *) outbuf,
				      (unsigned short *) tile, num_bands,
				      width, height, tile_width, tile_height,
				      base_y, base_x);
	  break;
      case RL2_SAMPLE_INT32:
	  copy_int32_outbuf_to_tile ((int *) outbuf,
				     (int *) tile, width, height, tile_width,
				     tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_outbuf_to_tile ((unsigned int *) outbuf,
				      (unsigned int *) tile, width, height,
				      tile_width, tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_FLOAT:
	  copy_float_outbuf_to_tile ((float *) outbuf,
				     (float *) tile, width, height, tile_width,
				     tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_outbuf_to_tile ((double *) outbuf,
				      (double *) tile, width, height,
				      tile_width, tile_height, base_y, base_x);
	  break;
      default:
	  copy_uint8_outbuf_to_tile ((unsigned char *) outbuf,
				     (unsigned char *) tile, num_bands, width,
				     height, tile_width, tile_height, base_y,
				     base_x);
	  break;
      };
}

static int
mismatching_size (unsigned short width, unsigned short height, double x_res,
		  double y_res, double minx, double miny, double maxx,
		  double maxy)
{
/* checking if the image size and the map extent do match */
    double ext_x = (double) width * x_res;
    double ext_y = (double) height * y_res;
    double img_x = maxx - minx;
    double img_y = maxy = miny;
    double confidence;
    confidence = ext_x / 100.0;
    if (img_x < (ext_x - confidence) || img_x > (ext_x + confidence))
	return 0;
    confidence = ext_y / 100.0;
    if (img_y < (ext_y - confidence) || img_y > (ext_y + confidence))
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_export_geotiff_from_dbms (sqlite3 * handle, const char *dst_path,
			      rl2CoveragePtr cvg, double x_res, double y_res,
			      double minx, double miny, double maxx,
			      double maxy, unsigned short width,
			      unsigned short height, unsigned char compression,
			      unsigned short tile_sz, int with_worldfile)
{
/* exporting a GeoTIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
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
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
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

/* computing the sample size */
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

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, pixel_type, num_bands,
					palette, compression, 1,
					tile_sz, srid, minx, miny, maxx,
					maxy, xx_res, yy_res, with_worldfile);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 palette);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    if (with_worldfile)
      {
	  /* exporting the Worldfile */
	  if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	      goto error;
      }

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_worldfile_from_dbms (sqlite3 * handle, const char *dst_path,
				     rl2CoveragePtr cvg, double x_res,
				     double y_res, double minx, double miny,
				     double maxx, double maxy,
				     unsigned short width,
				     unsigned short height,
				     unsigned char compression,
				     unsigned short tile_sz)
{
/* exporting a TIFF+TFW from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
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
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
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

/* computing the sample size */
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

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, pixel_type,
					       num_bands, palette, compression,
					       1, tile_sz, srid, minx, miny,
					       maxx, maxy, xx_res, yy_res);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 palette);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

/* exporting the Worldfile */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	goto error;

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
			   rl2CoveragePtr cvg, double x_res, double y_res,
			   double minx, double miny, double maxx,
			   double maxy, unsigned short width,
			   unsigned short height, unsigned char compression,
			   unsigned short tile_sz)
{
/* exporting a plain TIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
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
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
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

/* computing the sample size */
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

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     pixel_type, num_bands, palette,
				     compression, 1, tile_sz);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 palette);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_ascii_grid_from_dbms (sqlite3 * handle, const char *dst_path,
				 rl2CoveragePtr cvg, double res,
				 double minx, double miny, double maxx,
				 double maxy, unsigned short width,
				 unsigned short height, int is_centered,
				 int decimal_digits)
{
/* exporting an ASCII Grid from the DBMS into the file-system */
    rl2PalettePtr palette = NULL;
    rl2AsciiGridDestinationPtr ascii = NULL;
    rl2PixelPtr pixel;
    unsigned char level;
    unsigned char scale;
    double xx_res = res;
    double yy_res = res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    double no_data = -9999.0;
    int base_x;
    int base_y;
    unsigned char *pixels = NULL;
    int pixels_size;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;

    if (pixel_type != RL2_PIXEL_DATAGRID || num_bands != 1)
	goto error;

    pixel = rl2_get_coverage_no_data (cvg);
    if (pixel != NULL)
      {
	  /* attempting to retrieve the NO-DATA value */
	  unsigned char st;
	  unsigned char pt;
	  unsigned char nb;
	  if (rl2_get_pixel_type (pixel, &st, &pt, &nb) == RL2_OK)
	    {
		if (st == RL2_SAMPLE_INT8)
		  {
		      char v8;
		      if (rl2_get_pixel_sample_int8 (pixel, &v8) == RL2_OK)
			  no_data = v8;
		  }
		if (st == RL2_SAMPLE_UINT8)
		  {
		      unsigned char vu8;
		      if (rl2_get_pixel_sample_uint8 (pixel, 0, &vu8) == RL2_OK)
			  no_data = vu8;
		  }
		if (st == RL2_SAMPLE_INT16)
		  {
		      short v16;
		      if (rl2_get_pixel_sample_int16 (pixel, &v16) == RL2_OK)
			  no_data = v16;
		  }
		if (st == RL2_SAMPLE_UINT16)
		  {
		      unsigned short vu16;
		      if (rl2_get_pixel_sample_uint16 (pixel, 0, &vu16) ==
			  RL2_OK)
			  no_data = vu16;
		  }
		if (st == RL2_SAMPLE_INT32)
		  {
		      int v32;
		      if (rl2_get_pixel_sample_int32 (pixel, &v32) == RL2_OK)
			  no_data = v32;
		  }
		if (st == RL2_SAMPLE_UINT32)
		  {
		      unsigned int vu32;
		      if (rl2_get_pixel_sample_uint32 (pixel, &vu32) == RL2_OK)
			  no_data = vu32;
		  }
		if (st == RL2_SAMPLE_FLOAT)
		  {
		      float vflt;
		      if (rl2_get_pixel_sample_float (pixel, &vflt) == RL2_OK)
			  no_data = vflt;
		  }
		if (st == RL2_SAMPLE_DOUBLE)
		  {
		      double vdbl;
		      if (rl2_get_pixel_sample_double (pixel, &vdbl) == RL2_OK)
			  no_data = vdbl;
		  }
	    }
      }

    if (rl2_get_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, res, res, &pixels,
	 &pixels_size, &palette) != RL2_OK)
	goto error;

    ascii =
	rl2_create_ascii_grid_destination (dst_path, width, height,
					   xx_res, minx, miny, is_centered,
					   no_data, decimal_digits, pixels,
					   pixels_size, sample_type);
    if (ascii == NULL)
	goto error;
    pixels = NULL;		/* pix-buffer ownership now belongs to the ASCII object */
/* writing the ASCII Grid header */
    if (rl2_write_ascii_grid_header (ascii) != RL2_OK)
	goto error;
    for (base_y = 0; base_y < height; base_y++)
      {
	  /* exporting all scanlines from the output buffer */
	  unsigned short line_no;
	  if (rl2_write_ascii_grid_scanline (ascii, &line_no) != RL2_OK)
	      goto error;
      }

    rl2_destroy_ascii_grid_destination (ascii);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_OK;

  error:
    if (ascii != NULL)
	rl2_destroy_ascii_grid_destination (ascii);
    if (pixels != NULL)
	free (pixels);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}
