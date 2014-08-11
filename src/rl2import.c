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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <time.h>

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
#include "rasterlite2/rl2graphics.h"
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

static int
do_insert_tile (sqlite3 * handle, unsigned char *blob_odd, int blob_odd_sz,
		unsigned char *blob_even, int blob_even_sz,
		sqlite3_int64 section_id, int srid, double res_x, double res_y,
		unsigned int tile_w, unsigned int tile_h, double miny,
		double maxx, double *tile_minx, double *tile_miny,
		double *tile_maxx, double *tile_maxy, rl2PalettePtr aux_palette,
		rl2PixelPtr no_data, sqlite3_stmt * stmt_tils,
		sqlite3_stmt * stmt_data, rl2RasterStatisticsPtr section_stats)
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

RL2_PRIVATE void
compute_aggregate_sq_diff (rl2RasterStatisticsPtr section_stats)
{
/* updating aggregate sum_sq_diff */
    int ib;
    rl2PoolVariancePtr pV;
    rl2PrivBandStatisticsPtr st_band;
    rl2PrivRasterStatisticsPtr st = (rl2PrivRasterStatisticsPtr) section_stats;
    if (st == NULL)
	return;

    for (ib = 0; ib < st->nBands; ib++)
      {
	  double sum_var = 0.0;
	  st_band = st->band_stats + ib;
	  pV = st_band->first;
	  while (pV != NULL)
	    {
		sum_var += (pV->count - 1.0) * pV->variance;
		pV = pV->next;
	    }
	  st_band->sum_sq_diff = sum_var;
      }
}

static int
do_import_ascii_grid (sqlite3 * handle, const char *src_path,
		      rl2CoveragePtr cvg, const char *section, int srid,
		      unsigned int tile_w, unsigned int tile_h,
		      int pyramidize, unsigned char sample_type,
		      unsigned char compression, sqlite3_stmt * stmt_data,
		      sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
		      sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect,
		      int verbose, int current, int total)
{
/* importing an ASCII Data Grid file */
    int ret;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2AsciiGridOriginPtr origin = NULL;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
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
    double base_res_x;
    double base_res_y;
    double confidence;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;
    time_t start;
    time_t now;
    time_t diff;
    int mins;
    int secs;
    char *xml_summary = NULL;

    time (&start);
    if (rl2_get_coverage_resolution (cvg, &base_res_x, &base_res_y) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Unknown Coverage Resolution\n");
	  goto error;
      }
    origin = rl2_create_ascii_grid_origin (src_path, srid, sample_type);
    if (origin == NULL)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid ASCII Grid Origin: %s\n", src_path);
	  goto error;
      }
    xml_summary = rl2_build_ascii_xml_summary (origin);

    if (total > 1)
	printf ("%d/%d) Importing: %s\n", current, total,
		rl2_get_ascii_grid_origin_path (origin));
    else
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
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  if (res_x != coverage->hResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		goto error;
	    }
	  if (res_y != coverage->vResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		goto error;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (res_x < (coverage->hResolution - confidence)
	      || res_x > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Permissive) !!!\n");
		goto error;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (res_y < (coverage->vResolution - confidence)
	      || res_y > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Permissive) !!!\n");
		goto error;
	    }
      }

    if (rl2_eval_ascii_grid_origin_compatibility (cvg, origin, verbose) !=
	RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/ASCII mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 xml_summary, coverage->sectionPaths, coverage->sectionMD5,
	 coverage->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, 1);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (coverage->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_ascii_grid_origin (cvg, origin, row, col,
							 verbose);
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
		     section_id, srid, res_x, res_y, tile_w, tile_h, miny,
		     maxx, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
	  if (verbose && isatty (STDOUT_FILENO))
	    {
		printf (">> Imported row %u of %u\r", row + tile_h, height);
	    }
      }

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_ascii_grid_origin (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;
    time (&now);
    diff = now - start;
    mins = diff / 60;
    secs = diff - (mins * 60);
    printf (">> Grid succesfully imported in: %d mins %02d secs\n", mins, secs);

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid (handle, coverage_name, section_id, 1)
	      != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

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
check_jpeg_origin_compatibility (rl2RasterPtr raster, rl2CoveragePtr coverage,
				 unsigned int *width, unsigned int *height,
				 unsigned char *forced_conversion)
{
/* checking if the JPEG and the Coverage are mutually compatible */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    if (rst == NULL || cvg == NULL)
	return 0;
    if (rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_GRAYSCALE && rst->nBands == 1)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_GRAYSCALE_TO_RGB;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT8 && rst->pixelType == RL2_PIXEL_RGB
	&& rst->nBands == 3)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_RGB_TO_GRAYSCALE;
		return 1;
	    }
      }
    return 0;
}

RL2_DECLARE char *
rl2_build_worldfile_path (const char *path, const char *suffix)
{
/* building a WorldFile path */
    char *wf_path;
    const char *x = NULL;
    const char *p = path;
    int len;

    if (path == NULL || suffix == NULL)
	return NULL;
    len = strlen (path);
    len -= 1;
    while (*p != '\0')
      {
	  if (*p == '.')
	      x = p;
	  p++;
      }
    if (x > path)
	len = x - path;
    wf_path = malloc (len + strlen (suffix) + 1);
    memcpy (wf_path, path, len);
    strcpy (wf_path + len, suffix);
    return wf_path;
}

static int
read_jgw_worldfile (const char *src_path, double *minx, double *maxy,
		    double *pres_x, double *pres_y)
{
/* attempting to retrieve georeferencing from a JPEG+JGW origin */
    FILE *jgw = NULL;
    double res_x;
    double res_y;
    double x;
    double y;
    char *jgw_path = NULL;

    jgw_path = rl2_build_worldfile_path (src_path, ".jgw");
    if (jgw_path == NULL)
	goto error;
    jgw = fopen (jgw_path, "r");
    free (jgw_path);
    jgw_path = NULL;
    if (jgw == NULL)
      {
	  /* trying the ".jpgw" suffix */
	  jgw_path = rl2_build_worldfile_path (src_path, ".jpgw");
	  if (jgw_path == NULL)
	      goto error;
	  jgw = fopen (jgw_path, "r");
	  free (jgw_path);
      }
    if (jgw == NULL)
      {
	  /* trying the ".wld" suffix */
	  jgw_path = rl2_build_worldfile_path (src_path, ".wld");
	  if (jgw_path == NULL)
	      goto error;
	  jgw = fopen (jgw_path, "r");
	  free (jgw_path);
      }
    if (jgw == NULL)
	goto error;
    if (!parse_worldfile (jgw, &x, &y, &res_x, &res_y))
	goto error;
    fclose (jgw);
    *pres_x = res_x;
    *pres_y = res_y;
    *minx = x;
    *maxy = y;
    return 1;

  error:
    if (jgw_path != NULL)
	free (jgw_path);
    if (jgw != NULL)
	fclose (jgw);
    return 0;
}

static void
write_jgw_worldfile (const char *path, double minx, double maxy, double x_res,
		     double y_res)
{
/* exporting a JGW WorldFile */
    FILE *jgw = NULL;
    char *jgw_path = NULL;

    jgw_path = rl2_build_worldfile_path (path, ".jgw");
    if (jgw_path == NULL)
	goto error;
    jgw = fopen (jgw_path, "w");
    free (jgw_path);
    jgw_path = NULL;
    if (jgw == NULL)
	goto error;
    fprintf (jgw, "        %1.16f\n", x_res);
    fprintf (jgw, "        0.0\n");
    fprintf (jgw, "        0.0\n");
    fprintf (jgw, "        -%1.16f\n", y_res);
    fprintf (jgw, "        %1.16f\n", minx);
    fprintf (jgw, "        %1.16f\n", maxy);
    fclose (jgw);
    return;

  error:
    if (jgw_path != NULL)
	free (jgw_path);
    if (jgw != NULL)
	fclose (jgw);
}

static int
do_import_jpeg_image (sqlite3 * handle, const char *src_path,
		      rl2CoveragePtr cvg, const char *section, int srid,
		      unsigned int tile_w, unsigned int tile_h,
		      int pyramidize, unsigned char sample_type,
		      unsigned char num_bands, unsigned char compression,
		      sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
		      sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
		      sqlite3_stmt * stmt_upd_sect, int verbose, int current,
		      int total)
{
/* importing a JPEG image file [with optional WorldFile */
    rl2SectionPtr origin = NULL;
    rl2RasterPtr rst_in;
    rl2PrivRasterPtr raster_in;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
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
    int is_georeferenced = 0;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;
    unsigned char forced_conversion = RL2_CONVERT_NO;
    double base_res_x;
    double base_res_y;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    double confidence;
    time_t start;
    time_t now;
    time_t diff;
    int mins;
    int secs;
    char *xml_summary = NULL;

    if (rl2_get_coverage_resolution (cvg, &base_res_x, &base_res_y) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Unknown Coverage Resolution\n");
	  goto error;
      }
    origin = rl2_section_from_jpeg (src_path);
    if (origin == NULL)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid JPEG Origin: %s\n", src_path);
	  goto error;
      }
    time (&start);
    rst_in = rl2_get_section_raster (origin);
    if (!check_jpeg_origin_compatibility
	(rst_in, cvg, &width, &height, &forced_conversion))
	goto error;
    if (read_jgw_worldfile (src_path, &minx, &maxy, &res_x, &res_y))
      {
	  /* georeferenced JPEG */
	  maxx = minx + ((double) width * res_x);
	  miny = maxy - ((double) height * res_y);
	  is_georeferenced = 1;
      }
    else
      {
	  /* not georeferenced JPEG */
	  if (srid != -1)
	      goto error;
	  minx = 0.0;
	  miny = 0.0;
	  maxx = width - 1.0;
	  maxy = height - 1.0;
	  res_x = 1.0;
	  res_y = 1.0;
      }
    raster_in = (rl2PrivRasterPtr) rst_in;
    xml_summary =
	rl2_build_jpeg_xml_summary (width, height, raster_in->pixelType,
				    is_georeferenced, res_x, res_y, minx, miny,
				    maxx, maxy);

    if (total > 1)
	printf ("%d/%d) Importing: %s\n", current, total, src_path);
    else
	printf ("Importing: %s\n", src_path);
    printf ("------------------\n");
    printf ("    Image Size (pixels): %d x %d\n", width, height);
    printf ("                   SRID: %d\n", srid);
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
    dumb1 = formatFloat (res_x);
    dumb2 = formatFloat (res_y);
    printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    if (coverage->Srid != srid)
      {
	  if (verbose)
	      fprintf (stderr, "Mismatching SRID !!!\n");
	  goto error;
      }
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  if (res_x != coverage->hResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		goto error;
	    }
	  if (res_y != coverage->vResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		goto error;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (res_x < (coverage->hResolution - confidence)
	      || res_x > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Permissive) !!!\n");
		goto error;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (res_y < (coverage->vResolution - confidence)
	      || res_y > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution !(Permissive) !!\n");
		goto error;
	    }
      }

    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 xml_summary, coverage->sectionPaths, coverage->sectionMD5,
	 coverage->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (coverage->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_jpeg_origin (cvg, rst_in, row, col,
						   forced_conversion, verbose);
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
		     section_id, srid, res_x, res_y, tile_w, tile_h, miny,
		     maxx, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
	  if (verbose && isatty (STDOUT_FILENO))
	    {
		printf (">> Imported row %u of %u\r", row + tile_h, height);
	    }
      }

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_section (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;
    time (&now);
    diff = now - start;
    mins = diff / 60;
    secs = diff - (mins * 60);
    printf (">> Image succesfully imported in: %d mins %02d secs\n", mins,
	    secs);

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid (handle, coverage_name, section_id, 1)
	      != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

    return 1;

  error:
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    if (origin != NULL)
	rl2_destroy_section (origin);
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
is_jpeg_image (const char *path)
{
/* testing for a JPEG image */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".jpg") == 0)
	      return 1;
      }
    return 0;
}

static int
do_import_file (sqlite3 * handle, const char *src_path,
		rl2CoveragePtr cvg, const char *section, int worldfile,
		int force_srid, int pyramidize, unsigned char sample_type,
		unsigned char pixel_type, unsigned char num_bands,
		unsigned int tile_w, unsigned int tile_h,
		unsigned char compression, int quality,
		sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
		sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
		sqlite3_stmt * stmt_upd_sect, int verbose, int current,
		int total)
{
/* importing a single Source file */
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    int ret;
    rl2TiffOriginPtr origin = NULL;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr aux_palette = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    int srid;
    int xsrid;
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
    double base_res_x;
    double base_res_y;
    double confidence;
    time_t start;
    time_t now;
    time_t diff;
    int mins;
    int secs;
    char *xml_summary = NULL;

    if (is_ascii_grid (src_path))
	return do_import_ascii_grid (handle, src_path, cvg, section, force_srid,
				     tile_w, tile_h, pyramidize, sample_type,
				     compression, stmt_data, stmt_tils,
				     stmt_sect, stmt_levl,
				     stmt_upd_sect, verbose, current, total);

    if (is_jpeg_image (src_path))
	return do_import_jpeg_image (handle, src_path, cvg, section, force_srid,
				     tile_w, tile_h, pyramidize, sample_type,
				     num_bands, compression, stmt_data,
				     stmt_tils, stmt_sect, stmt_levl,
				     stmt_upd_sect, verbose, current, total);

    time (&start);
    if (rl2_get_coverage_resolution (cvg, &base_res_x, &base_res_y) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Unknown Coverage Resolution\n");
	  goto error;
      }
    if (worldfile)
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_WORLDFILE, force_srid,
				    sample_type, pixel_type, num_bands);
    else
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_GEOTIFF, force_srid,
				    sample_type, pixel_type, num_bands);
    if (origin == NULL)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid TIFF Origin: %s\n", src_path);
	  goto error;
      }
    if (rl2_get_coverage_srid (cvg, &xsrid) == RL2_OK)
      {
	  if (xsrid == RL2_GEOREFERENCING_NONE)
	      rl2_set_tiff_origin_not_referenced (origin);
      }
    xml_summary = rl2_build_tiff_xml_summary (origin);

    if (total > 1)
	printf ("%d/%d) Importing: %s\n", current, total,
		rl2_get_tiff_origin_path (origin));
    else
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
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  if (res_x != coverage->hResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		goto error;
	    }
	  if (res_y != coverage->vResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		goto error;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (res_x < (coverage->hResolution - confidence)
	      || res_x > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Permissive) !!!\n");
		goto error;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (res_y < (coverage->vResolution - confidence)
	      || res_y > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution !(Permissive) !!\n");
		goto error;
	    }
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

    if (rl2_eval_tiff_origin_compatibility (cvg, origin, force_srid, verbose) !=
	RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 xml_summary, coverage->sectionPaths, coverage->sectionMD5,
	 coverage->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (coverage->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_tiff_origin (cvg, origin, row, col, srid,
						   verbose);
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
		     section_id, srid, res_x, res_y, tile_w, tile_h, miny,
		     maxx, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     aux_palette, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
	  if (verbose && isatty (STDOUT_FILENO))
	    {
		printf (">> Imported row %u of %u\r", row + tile_h, height);
	    }
      }

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_tiff_origin (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;
    time (&now);
    diff = now - start;
    mins = diff / 60;
    secs = diff - (mins * 60);
    printf (">> Image succesfully imported in: %d mins %02d secs\n", mins,
	    secs);

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid (handle, coverage_name, section_id, 1)
	      != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

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
    int len;
    char *ext;
    int match = 0;
    if (file_ext == NULL)
	return 0;

    len = strlen (file_ext);
    if (*file_ext == '.')
      {
	  /* file extension starts with dot */
	  ext = malloc (len + 1);
	  strcpy (ext, file_ext);
      }
    else
      {
	  /* file extension doesn't start with dot */
	  ext = malloc (len + 2);
	  *ext = '.';
	  strcpy (ext + 1, file_ext);
      }
    while (*p != '\0')
      {
	  if (*p == '.')
	      mark = p;
	  p++;
      }
    if (mark == NULL)
      {
	  free (ext);
	  return 0;
      }
    match = strcasecmp (mark, ext);
    free (ext);
    if (match == 0)
	return 1;
    return 0;
}

static int
do_import_dir (sqlite3 * handle, const char *dir_path, const char *file_ext,
	       rl2CoveragePtr cvg, const char *section, int worldfile,
	       int force_srid, int pyramidize, unsigned char sample_type,
	       unsigned char pixel_type, unsigned char num_bands,
	       unsigned int tile_w, unsigned int tile_h,
	       unsigned char compression, int quality, sqlite3_stmt * stmt_data,
	       sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
	       sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect,
	       int verbose)
{
/* importing a whole directory */
#if defined(_WIN32) && !defined(__MINGW32__)
/* Visual Studio .NET */
    struct _finddata_t c_file;
    intptr_t hFile;
    int cnt = 0;
    int total = 0;
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
		    total++;
		if (_findnext (hFile, &c_file) != 0)
		    break;
	    }
	  _findclose (hFile);
	  if ((hFile = _findfirst (search, &c_file)) == -1L)
	      ;
	  else
	    {
		while (1)
		  {
		      if ((c_file.attrib & _A_RDONLY) == _A_RDONLY
			  || (c_file.attrib & _A_NORMAL) == _A_NORMAL)
			{
			    path =
				sqlite3_mprintf ("%s/%s", dir_path,
						 c_file.name);
			    ret =
				do_import_file (handle, path, cvg, section,
						worldfile, force_srid,
						pyramidize, sample_type,
						pixel_type, num_bands, tile_w,
						tile_h, compression, quality,
						stmt_data, stmt_tils, stmt_sect,
						stmt_levl, stmt_upd_sect,
						verbose, cnt + 1, total);
			    sqlite3_free (path);
			    if (!ret)
				goto error;
			    cnt++;
			}
		      if (_findnext (hFile, &c_file) != 0)
			  break;
		  }
	      error:
		_findclose (hFile);
	    }
	  sqlite3_free (search);
	  return cnt;
#else
/* not Visual Studio .NET */
    int cnt = 0;
    int total = 0;
    char *path;
    struct dirent *entry;
    int ret;
    DIR *dir = opendir (dir_path);
    if (!dir)
	return 0;
    while (1)
      {
	  /* counting how many valid entries */
	  entry = readdir (dir);
	  if (!entry)
	      break;
	  if (!check_extension_match (entry->d_name, file_ext))
	      continue;
	  total++;
      }
    rewinddir (dir);
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
			      stmt_tils, stmt_sect, stmt_levl,
			      stmt_upd_sect, verbose, cnt + 1, total);
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
		  int worldfile, int force_srid, int pyramidize, int verbose)
{
/* main IMPORT Raster function */
    rl2PrivCoveragePtr privcvg = (rl2PrivCoveragePtr) cvg;
    int ret;
    char *sql;
    const char *coverage;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned int tile_w;
    unsigned int tile_h;
    unsigned char compression;
    int quality;
    char *table;
    char *xtable;
    unsigned int tileWidth;
    unsigned int tileHeight;
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
	 "md5_checksum, summary, width, height, geometry) "
	 "VALUES (NULL, ?, ?, ?, XB_Create(?), ?, ?, ?)", xtable);
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

    if (privcvg->mixedResolutions)
      {
	  /* mixed resolutions Coverage */
	  table = sqlite3_mprintf ("%s_section_levels", coverage);
	  xtable = gaiaDoubleQuotedSql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf
	      ("INSERT OR IGNORE INTO \"%s\" (section_id, pyramid_level, "
	       "x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	       "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	       "VALUES (?, 0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
	  free (xtable);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("INSERT INTO section_levels SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }
    else
      {
	  /* single resolution Coverage */
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
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("INSERT INTO levels SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
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
	       stmt_levl, stmt_upd_sect, verbose, -1, -1))
	      goto error;
      }
    else
      {
	  /* importing all Image files from a whole directory */
	  if (!do_import_dir
	      (handle, dir_path, file_ext, cvg, section, worldfile, force_srid,
	       pyramidize, sample_type, pixel_type, num_bands, tile_w, tile_h,
	       compression, quality, stmt_data, stmt_tils, stmt_sect,
	       stmt_levl, stmt_upd_sect, verbose))
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
			   int force_srid, int pyramidize, int verbose)
{
/* importing a single Raster file */
    if (!do_import_common
	(handle, src_path, NULL, NULL, coverage, NULL, worldfile, force_srid,
	 pyramidize, verbose))
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_load_mrasters_into_dbms (sqlite3 * handle, const char *dir_path,
			     const char *file_ext,
			     rl2CoveragePtr coverage, int worldfile,
			     int force_srid, int pyramidize, int verbose)
{
/* importing multiple Raster files from dir */
    if (!do_import_common
	(handle, NULL, dir_path, file_ext, coverage, NULL, worldfile,
	 force_srid, pyramidize, verbose))
	return RL2_ERROR;
    return RL2_OK;
}

static int
mismatching_size (unsigned int width, unsigned int height, double x_res,
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

static void
copy_int8_outbuf_to_tile (const char *outbuf, char *tile,
			  unsigned int width,
			  unsigned int height,
			  unsigned int tile_width,
			  unsigned int tile_height, unsigned int base_y,
			  unsigned int base_x)
{
/* copying INT8 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			   unsigned char num_bands, unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying UINT8 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			   unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying INT16 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			    unsigned char num_bands, unsigned int width,
			    unsigned int height,
			    unsigned int tile_width,
			    unsigned int tile_height, unsigned int base_y,
			    unsigned int base_x)
{
/* copying UINT16 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			   unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying INT32 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			    unsigned int width,
			    unsigned int height,
			    unsigned int tile_width,
			    unsigned int tile_height, int base_y, int base_x)
{
/* copying UINT32 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			   unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying FLOAT pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			    unsigned int width,
			    unsigned int height,
			    unsigned int tile_width,
			    unsigned int tile_height, unsigned int base_y,
			    unsigned int base_x)
{
/* copying DOUBLE pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
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
			  unsigned int width, unsigned int height,
			  unsigned int tile_width, unsigned int tile_height,
			  unsigned int base_y, unsigned int base_x)
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
export_geotiff_common (sqlite3 * handle, const char *dst_path,
		       rl2CoveragePtr cvg, int by_section,
		       sqlite3_int64 section_id, double x_res, double y_res,
		       double minx, double miny, double maxx, double maxy,
		       unsigned int width, unsigned int height,
		       unsigned char compression, unsigned int tile_sz,
		       int with_worldfile)
{
/* exporting a GeoTIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;

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
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (by_section)
      {
	  /* just a single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, &outbuf, &outbuf_size, &palette,
	       pixel_type) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, &outbuf, &outbuf_size, &palette, pixel_type) != RL2_OK)
	      goto error;
      }

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
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
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
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_geotiff_from_dbms (sqlite3 * handle, const char *dst_path,
			      rl2CoveragePtr cvg, double x_res, double y_res,
			      double minx, double miny, double maxx,
			      double maxy, unsigned int width,
			      unsigned int height, unsigned char compression,
			      unsigned int tile_sz, int with_worldfile)
{
/* exporting a GeoTIFF from the DBMS into the file-system */
    return export_geotiff_common (handle, dst_path, cvg, 0, 0, x_res, y_res,
				  minx, miny, maxx, maxy, width, height,
				  compression, tile_sz, with_worldfile);
}

RL2_DECLARE int
rl2_export_section_geotiff_from_dbms (sqlite3 * handle, const char *dst_path,
				      rl2CoveragePtr cvg,
				      sqlite3_int64 section_id, double x_res,
				      double y_res, double minx, double miny,
				      double maxx, double maxy,
				      unsigned int width, unsigned int height,
				      unsigned char compression,
				      unsigned int tile_sz, int with_worldfile)
{
/* exporting a GeoTIFF - Section*/
    return export_geotiff_common (handle, dst_path, cvg, 1, section_id, x_res,
				  y_res, minx, miny, maxx, maxy, width, height,
				  compression, tile_sz, with_worldfile);
}

static int
export_tiff_worlfile_common (sqlite3 * handle, const char *dst_path,
			     rl2CoveragePtr cvg, int by_section,
			     sqlite3_int64 section_id, double x_res,
			     double y_res, double minx, double miny,
			     double maxx, double maxy, unsigned int width,
			     unsigned int height, unsigned char compression,
			     unsigned int tile_sz)
{
/* exporting a TIFF+TFW common implementation */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;

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
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (by_section)
      {
	  /* just a single select Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, &outbuf, &outbuf_size, &palette,
	       pixel_type) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, &outbuf, &outbuf_size, &palette, pixel_type) != RL2_OK)
	      goto error;
      }

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
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
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
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_worldfile_from_dbms (sqlite3 * handle, const char *dst_path,
				     rl2CoveragePtr cvg, double x_res,
				     double y_res, double minx, double miny,
				     double maxx, double maxy,
				     unsigned int width,
				     unsigned int height,
				     unsigned char compression,
				     unsigned int tile_sz)
{
/* exporting a TIFF+TFW from the DBMS into the file-system */
    return export_tiff_worlfile_common (handle, dst_path, cvg, 0, 0, x_res,
					y_res, minx, miny, maxx, maxy, width,
					height, compression, tile_sz);
}

RL2_DECLARE int
rl2_export_section_tiff_worldfile_from_dbms (sqlite3 * handle,
					     const char *dst_path,
					     rl2CoveragePtr cvg,
					     sqlite3_int64 section_id,
					     double x_res, double y_res,
					     double minx, double miny,
					     double maxx, double maxy,
					     unsigned int width,
					     unsigned int height,
					     unsigned char compression,
					     unsigned int tile_sz)
{
/* exporting a TIFF+TFW - single Section */
    return export_tiff_worlfile_common (handle, dst_path, cvg, 1, section_id,
					x_res, y_res, minx, miny, maxx, maxy,
					width, height, compression, tile_sz);
}

static int
export_tiff_common (sqlite3 * handle, const char *dst_path,
		    rl2CoveragePtr cvg, int by_section,
		    sqlite3_int64 section_id, double x_res, double y_res,
		    double minx, double miny, double maxx, double maxy,
		    unsigned int width, unsigned int height,
		    unsigned char compression, unsigned int tile_sz)
{
/* exporting a plain TIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;

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
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_UINT8
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (by_section)
      {
	  /* just a single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, &outbuf, &outbuf_size, &palette,
	       pixel_type) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, &outbuf, &outbuf_size, &palette, pixel_type) != RL2_OK)
	      goto error;
      }

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
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
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
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
			   rl2CoveragePtr cvg, double x_res, double y_res,
			   double minx, double miny, double maxx,
			   double maxy, unsigned int width,
			   unsigned int height, unsigned char compression,
			   unsigned int tile_sz)
{
/* exporting a plain TIFF from the DBMS into the file-system */
    return export_tiff_common (handle, dst_path, cvg, 0, 0, x_res, y_res, minx,
			       miny, maxx, maxy, width, height, compression,
			       tile_sz);
}

RL2_DECLARE int
rl2_export_section_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
				   rl2CoveragePtr cvg, sqlite3_int64 section_id,
				   double x_res, double y_res, double minx,
				   double miny, double maxx, double maxy,
				   unsigned int width, unsigned int height,
				   unsigned char compression,
				   unsigned int tile_sz)
{
/* exporting a plain TIFF - single Section*/
    return export_tiff_common (handle, dst_path, cvg, 1, section_id, x_res,
			       y_res, minx, miny, maxx, maxy, width, height,
			       compression, tile_sz);
}

static int
export_triple_band_geotiff_common (int by_section, sqlite3 * handle,
				   const char *dst_path,
				   rl2CoveragePtr cvg, sqlite3_int64 section_id,
				   double x_res, double y_res, double minx,
				   double miny, double maxx, double maxy,
				   unsigned int width, unsigned int height,
				   unsigned char red_band,
				   unsigned char green_band,
				   unsigned char blue_band,
				   unsigned char compression,
				   unsigned int tile_sz, int with_worldfile)
{
/* exporting a Band-Composed GeoTIFF - common implementation */
    rl2RasterPtr raster = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_triple_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, red_band, green_band, blue_band, &outbuf,
	       &outbuf_size, no_data) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_triple_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	       no_data) != RL2_OK)
	      goto error;
      }

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, RL2_PIXEL_RGB, 3,
					NULL, compression, 1, tile_sz, srid,
					minx, miny, maxx, maxy, xx_res, yy_res,
					with_worldfile);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
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
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_geotiff_from_dbms (sqlite3 * handle,
					  const char *dst_path,
					  rl2CoveragePtr cvg, double x_res,
					  double y_res, double minx,
					  double miny, double maxx,
					  double maxy, unsigned int width,
					  unsigned int height,
					  unsigned char red_band,
					  unsigned char green_band,
					  unsigned char blue_band,
					  unsigned char compression,
					  unsigned int tile_sz,
					  int with_worldfile)
{
/* exporting a Band-Composed GeoTIFF from the DBMS into the file-system */
    return export_triple_band_geotiff_common (0, handle, dst_path, cvg, 0,
					      x_res, y_res, minx, miny, maxx,
					      maxy, width, height, red_band,
					      green_band, blue_band,
					      compression, tile_sz,
					      with_worldfile);
}

RL2_DECLARE int
rl2_export_section_triple_band_geotiff_from_dbms (sqlite3 * handle,
						  const char *dst_path,
						  rl2CoveragePtr cvg,
						  sqlite3_int64 section_id,
						  double x_res, double y_res,
						  double minx, double miny,
						  double maxx, double maxy,
						  unsigned int width,
						  unsigned int height,
						  unsigned char red_band,
						  unsigned char green_band,
						  unsigned char blue_band,
						  unsigned char compression,
						  unsigned int tile_sz,
						  int with_worldfile)
{
/* exporting a Band-Composed GeoTIFF - Section */
    return export_triple_band_geotiff_common (1, handle, dst_path, cvg,
					      section_id, x_res, y_res, minx,
					      miny, maxx, maxy, width, height,
					      red_band, green_band, blue_band,
					      compression, tile_sz,
					      with_worldfile);
}

static int
export_mono_band_geotiff_common (int by_section, sqlite3 * handle,
				 const char *dst_path,
				 rl2CoveragePtr cvg, sqlite3_int64 section_id,
				 double x_res, double y_res, double minx,
				 double miny, double maxx, double maxy,
				 unsigned int width, unsigned int height,
				 unsigned char mono_band,
				 unsigned char compression,
				 unsigned int tile_sz, int with_worldfile)
{
/* exporting a Mono-Band GeoTIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data_mono = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;
    unsigned char out_pixel;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_mono = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_mono, mono_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_mono_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, mono_band, &outbuf, &outbuf_size,
	       no_data) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_mono_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, mono_band, &outbuf, &outbuf_size, no_data) != RL2_OK)
	      goto error;
      }

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, out_pixel, 1,
					NULL, compression, 1, tile_sz, srid,
					minx, miny, maxx, maxy, xx_res, yy_res,
					with_worldfile);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
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
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_geotiff_from_dbms (sqlite3 * handle,
					const char *dst_path,
					rl2CoveragePtr cvg, double x_res,
					double y_res, double minx,
					double miny, double maxx,
					double maxy, unsigned int width,
					unsigned int height,
					unsigned char mono_band,
					unsigned char compression,
					unsigned int tile_sz,
					int with_worldfile)
{
/* exporting a Mono-Band GeoTIFF from the DBMS into the file-system */
    return export_mono_band_geotiff_common (0, handle, dst_path, cvg, 0, x_res,
					    y_res, minx, miny, maxx, maxy,
					    width, height, mono_band,
					    compression, tile_sz,
					    with_worldfile);
}

RL2_DECLARE int
rl2_export_section_mono_band_geotiff_from_dbms (sqlite3 * handle,
						const char *dst_path,
						rl2CoveragePtr cvg,
						sqlite3_int64 section_id,
						double x_res, double y_res,
						double minx, double miny,
						double maxx, double maxy,
						unsigned int width,
						unsigned int height,
						unsigned char mono_band,
						unsigned char compression,
						unsigned int tile_sz,
						int with_worldfile)
{
/* exporting a Mono-Band GeoTIFF - Section */
    return export_mono_band_geotiff_common (1, handle, dst_path, cvg,
					    section_id, x_res, y_res, minx,
					    miny, maxx, maxy, width, height,
					    mono_band, compression, tile_sz,
					    with_worldfile);
}

static int
export_triple_band_tiff_worldfile_common (int by_section, sqlite3 * handle,
					  const char *dst_path,
					  rl2CoveragePtr cvg,
					  sqlite3_int64 section_id,
					  double x_res, double y_res,
					  double minx, double miny, double maxx,
					  double maxy, unsigned int width,
					  unsigned int height,
					  unsigned char red_band,
					  unsigned char green_band,
					  unsigned char blue_band,
					  unsigned char compression,
					  unsigned int tile_sz)
{
/* exporting a Band-Composed TIFF+TFW common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_triple_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, red_band, green_band, blue_band, &outbuf,
	       &outbuf_size, no_data) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_triple_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	       no_data) != RL2_OK)
	      goto error;
      }

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, RL2_PIXEL_RGB,
					       3, NULL, compression, 1, tile_sz,
					       srid, minx, miny, maxx, maxy,
					       xx_res, yy_res);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
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
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_tiff_worldfile_from_dbms (sqlite3 * handle,
						 const char *dst_path,
						 rl2CoveragePtr cvg,
						 double x_res, double y_res,
						 double minx, double miny,
						 double maxx, double maxy,
						 unsigned int width,
						 unsigned int height,
						 unsigned char red_band,
						 unsigned char green_band,
						 unsigned char blue_band,
						 unsigned char compression,
						 unsigned int tile_sz)
{
/* exporting a Band-Composed TIFF+TFW from the DBMS into the file-system */
    return export_triple_band_tiff_worldfile_common (0, handle, dst_path, cvg,
						     0, x_res, y_res, minx,
						     miny, maxx, maxy, width,
						     height, red_band,
						     green_band, blue_band,
						     compression, tile_sz);
}

RL2_DECLARE int
rl2_export_section_triple_band_tiff_worldfile_from_dbms (sqlite3 * handle,
							 const char *dst_path,
							 rl2CoveragePtr cvg,
							 sqlite3_int64
							 section_id,
							 double x_res,
							 double y_res,
							 double minx,
							 double miny,
							 double maxx,
							 double maxy,
							 unsigned int width,
							 unsigned int height,
							 unsigned char red_band,
							 unsigned char
							 green_band,
							 unsigned char
							 blue_band,
							 unsigned char
							 compression,
							 unsigned int tile_sz)
{
/* exporting a Band-Composed TIFF+TFW - Sction */
    return export_triple_band_tiff_worldfile_common (1, handle, dst_path, cvg,
						     section_id, x_res, y_res,
						     minx, miny, maxx, maxy,
						     width, height, red_band,
						     green_band, blue_band,
						     compression, tile_sz);
}

static int
export_mono_band_tiff_worldfile_common (int by_section, sqlite3 * handle,
					const char *dst_path,
					rl2CoveragePtr cvg,
					sqlite3_int64 section_id, double x_res,
					double y_res, double minx, double miny,
					double maxx, double maxy,
					unsigned int width, unsigned int height,
					unsigned char mono_band,
					unsigned char compression,
					unsigned int tile_sz)
{
/* exporting a Mono-Band TIFF+TFW - common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;
    unsigned char out_pixel;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_multi, mono_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_mono_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, mono_band, &outbuf, &outbuf_size,
	       no_data) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_mono_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, mono_band, &outbuf, &outbuf_size, no_data) != RL2_OK)
	      goto error;
      }

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, out_pixel,
					       1, NULL, compression, 1, tile_sz,
					       srid, minx, miny, maxx, maxy,
					       xx_res, yy_res);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
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
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_tiff_worldfile_from_dbms (sqlite3 * handle,
					       const char *dst_path,
					       rl2CoveragePtr cvg,
					       double x_res, double y_res,
					       double minx, double miny,
					       double maxx, double maxy,
					       unsigned int width,
					       unsigned int height,
					       unsigned char mono_band,
					       unsigned char compression,
					       unsigned int tile_sz)
{
/* exporting a Mono-Band TIFF+TFW from the DBMS into the file-system */
    return export_mono_band_tiff_worldfile_common (0, handle, dst_path, cvg, 0,
						   x_res, y_res, minx, miny,
						   maxx, maxy, width, height,
						   mono_band, compression,
						   tile_sz);
}

RL2_DECLARE int
rl2_export_section_mono_band_tiff_worldfile_from_dbms (sqlite3 * handle,
						       const char *dst_path,
						       rl2CoveragePtr cvg,
						       sqlite3_int64 section_id,
						       double x_res,
						       double y_res,
						       double minx, double miny,
						       double maxx, double maxy,
						       unsigned int width,
						       unsigned int height,
						       unsigned char mono_band,
						       unsigned char
						       compression,
						       unsigned int tile_sz)
{
/* exporting a Mono-Band TIFF+TFW - Section */
    return export_mono_band_tiff_worldfile_common (1, handle, dst_path, cvg,
						   section_id, x_res, y_res,
						   minx, miny, maxx, maxy,
						   width, height, mono_band,
						   compression, tile_sz);
}

static int
export_triple_band_tiff_common (int by_section, sqlite3 * handle,
				const char *dst_path, rl2CoveragePtr cvg,
				sqlite3_int64 section_id, double x_res,
				double y_res, double minx, double miny,
				double maxx, double maxy, unsigned int width,
				unsigned int height, unsigned char red_band,
				unsigned char green_band,
				unsigned char blue_band,
				unsigned char compression, unsigned int tile_sz)
{
/* exporting a plain Band-Composed TIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_triple_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, red_band, green_band, blue_band, &outbuf,
	       &outbuf_size, no_data) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_triple_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	       no_data) != RL2_OK)
	      goto error;
      }

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     RL2_PIXEL_RGB, 3, NULL,
				     compression, 1, tile_sz);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
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
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
				       rl2CoveragePtr cvg, double x_res,
				       double y_res, double minx, double miny,
				       double maxx, double maxy,
				       unsigned int width,
				       unsigned int height,
				       unsigned char red_band,
				       unsigned char green_band,
				       unsigned char blue_band,
				       unsigned char compression,
				       unsigned int tile_sz)
{
/* exporting a plain Band-Composed TIFF from the DBMS into the file-system */
    return export_triple_band_tiff_common (0, handle, dst_path, cvg, 0, x_res,
					   y_res, minx, miny, maxx, maxy, width,
					   height, red_band, green_band,
					   blue_band, compression, tile_sz);
}

RL2_DECLARE int
rl2_export_section_triple_band_tiff_from_dbms (sqlite3 * handle,
					       const char *dst_path,
					       rl2CoveragePtr cvg,
					       sqlite3_int64 section_id,
					       double x_res, double y_res,
					       double minx, double miny,
					       double maxx, double maxy,
					       unsigned int width,
					       unsigned int height,
					       unsigned char red_band,
					       unsigned char green_band,
					       unsigned char blue_band,
					       unsigned char compression,
					       unsigned int tile_sz)
{
/* exporting a plain Band-Composed TIFF - Section */
    return export_triple_band_tiff_common (1, handle, dst_path, cvg, section_id,
					   x_res, y_res, minx, miny, maxx, maxy,
					   width, height, red_band, green_band,
					   blue_band, compression, tile_sz);
}

static int
export_mono_band_tiff_common (int by_section, sqlite3 * handle,
			      const char *dst_path, rl2CoveragePtr cvg,
			      sqlite3_int64 section_id, double x_res,
			      double y_res, double minx, double miny,
			      double maxx, double maxy, unsigned int width,
			      unsigned int height, unsigned char mono_band,
			      unsigned char compression, unsigned int tile_sz)
{
/* exporting a plain Mono-Band TIFF - common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
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
    unsigned int base_x;
    unsigned int base_y;
    unsigned char out_pixel;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_multi, mono_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_mono_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, mono_band, &outbuf, &outbuf_size,
	       no_data) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_mono_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, mono_band, &outbuf, &outbuf_size, no_data) != RL2_OK)
	      goto error;
      }

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     out_pixel, 1, NULL,
				     compression, 1, tile_sz);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
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
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
				     rl2CoveragePtr cvg, double x_res,
				     double y_res, double minx, double miny,
				     double maxx, double maxy,
				     unsigned int width,
				     unsigned int height,
				     unsigned char mono_band,
				     unsigned char compression,
				     unsigned int tile_sz)
{
/* exporting a plain Mono-Band TIFF from the DBMS into the file-system */
    return export_mono_band_tiff_common (0, handle, dst_path, cvg, 0, x_res,
					 y_res, minx, miny, maxx, maxy, width,
					 height, mono_band, compression,
					 tile_sz);
}

RL2_DECLARE int
rl2_export_section_mono_band_tiff_from_dbms (sqlite3 * handle,
					     const char *dst_path,
					     rl2CoveragePtr cvg,
					     sqlite3_int64 section_id,
					     double x_res, double y_res,
					     double minx, double miny,
					     double maxx, double maxy,
					     unsigned int width,
					     unsigned int height,
					     unsigned char mono_band,
					     unsigned char compression,
					     unsigned int tile_sz)
{
/* exporting a plain Mono-Band TIFF from the DBMS - Section */
    return export_mono_band_tiff_common (1, handle, dst_path, cvg, section_id,
					 x_res, y_res, minx, miny, maxx, maxy,
					 width, height, mono_band, compression,
					 tile_sz);
}

static int
export_ascii_grid_common (int by_section, sqlite3 * handle,
			  const char *dst_path, rl2CoveragePtr cvg,
			  sqlite3_int64 section_id, double res, double minx,
			  double miny, double maxx, double maxy,
			  unsigned int width, unsigned int height,
			  int is_centered, int decimal_digits)
{
/* exporting an ASCII Grid common implementation */
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
    unsigned int base_y;
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

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       res, res, &pixels, &pixels_size, &palette,
	       RL2_PIXEL_DATAGRID) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, res, res,
	       &pixels, &pixels_size, &palette, RL2_PIXEL_DATAGRID) != RL2_OK)
	      goto error;
      }

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
	  unsigned int line_no;
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

RL2_DECLARE int
rl2_export_ascii_grid_from_dbms (sqlite3 * handle, const char *dst_path,
				 rl2CoveragePtr cvg, double res,
				 double minx, double miny, double maxx,
				 double maxy, unsigned int width,
				 unsigned int height, int is_centered,
				 int decimal_digits)
{
/* exporting an ASCII Grid from the DBMS into the file-system */
    return export_ascii_grid_common (0, handle, dst_path, cvg, 0, res, minx,
				     miny, maxx, maxy, width, height,
				     is_centered, decimal_digits);
}

RL2_DECLARE int
rl2_export_section_ascii_grid_from_dbms (sqlite3 * handle, const char *dst_path,
					 rl2CoveragePtr cvg,
					 sqlite3_int64 section_id, double res,
					 double minx, double miny, double maxx,
					 double maxy, unsigned int width,
					 unsigned int height, int is_centered,
					 int decimal_digits)
{
/* exporting an ASCII Grid - Section */
    return export_ascii_grid_common (1, handle, dst_path, cvg, section_id, res,
				     minx, miny, maxx, maxy, width, height,
				     is_centered, decimal_digits);
}

static int
export_jpeg_common (int by_section, sqlite3 * handle, const char *dst_path,
		    rl2CoveragePtr cvg, sqlite3_int64 section_id, double x_res,
		    double y_res, double minx, double miny,
		    double maxx, double maxy,
		    unsigned int width,
		    unsigned int height, int quality, int with_worldfile)
{
/* common implementation for Write JPEG */
    rl2SectionPtr section = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char *outbuf = NULL;
    int outbuf_size;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_GRAYSCALE
	&& num_bands == 1)
	;
    else if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_RGB
	     && num_bands == 3)
	;
    else
	goto error;

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, &outbuf, &outbuf_size, NULL,
	       pixel_type) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, &outbuf, &outbuf_size, NULL, pixel_type) != RL2_OK)
	      goto error;
      }

    raster =
	rl2_create_raster (width, height, sample_type, pixel_type, num_bands,
			   outbuf, outbuf_size, NULL, NULL, 0, NULL);
    outbuf = NULL;
    if (raster == NULL)
	goto error;
    section =
	rl2_create_section ("jpeg", RL2_COMPRESSION_JPEG, 256, 256, raster);
    raster = NULL;
    if (section == NULL)
	goto error;
    if (rl2_section_to_jpeg (section, dst_path, quality) != RL2_OK)
	goto error;

    if (with_worldfile)
      {
	  /* exporting the JGW WorldFile */
	  write_jgw_worldfile (dst_path, minx, maxy, x_res, y_res);
      }

    rl2_destroy_section (section);
    return RL2_OK;

  error:
    if (section != NULL)
	rl2_destroy_section (section);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (outbuf != NULL)
	free (outbuf);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_jpeg_from_dbms (sqlite3 * handle, const char *dst_path,
			   rl2CoveragePtr cvg, double x_res,
			   double y_res, double minx, double miny,
			   double maxx, double maxy,
			   unsigned int width,
			   unsigned int height, int quality, int with_worldfile)
{
/* exporting a JPEG (with possible JGW) from the DBMS into the file-system */
    return export_jpeg_common (0, handle, dst_path, cvg, 0, x_res, y_res, minx,
			       miny, maxx, maxy, width, height, quality,
			       with_worldfile);
}

RL2_DECLARE int
rl2_export_section_jpeg_from_dbms (sqlite3 * handle, const char *dst_path,
				   rl2CoveragePtr cvg, sqlite3_int64 section_id,
				   double x_res, double y_res, double minx,
				   double miny, double maxx, double maxy,
				   unsigned int width, unsigned int height,
				   int quality, int with_worldfile)
{
/* exporting a JPEG (with possible JGW) - Section */
    return export_jpeg_common (1, handle, dst_path, cvg, section_id, x_res,
			       y_res, minx, miny, maxx, maxy, width, height,
			       quality, with_worldfile);
}
