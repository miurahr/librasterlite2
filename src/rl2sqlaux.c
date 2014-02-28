/*

 rl2sqlaux -- private SQL helper methods

 version 0.1, 2014 February 28

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
#include <inttypes.h>

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
#include "rasterlite2/rl2wms.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

#include <spatialite/gaiaaux.h>

#define RL2_UNUSED() if (argc || argv) argc = argc;

RL2_PRIVATE int
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

RL2_PRIVATE int
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

RL2_PRIVATE rl2PixelPtr
default_nodata (unsigned char sample, unsigned char pixel,
		unsigned char num_bands)
{
/* creating a default NO-DATA value */
    int nb;
    rl2PixelPtr pxl = rl2_create_pixel (sample, pixel, num_bands);
    if (pxl == NULL)
	return NULL;
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  rl2_set_pixel_sample_1bit (pxl, 0);
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (sample)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_set_pixel_sample_1bit (pxl, 0);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_set_pixel_sample_2bit (pxl, 0);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_set_pixel_sample_4bit (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_set_pixel_sample_1bit (pxl, 1);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_set_pixel_sample_2bit (pxl, 3);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_set_pixel_sample_4bit (pxl, 15);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_RGB:
	  switch (sample)
	    {
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 255);
		rl2_set_pixel_sample_uint8 (pxl, 1, 255);
		rl2_set_pixel_sample_uint8 (pxl, 2, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		rl2_set_pixel_sample_uint16 (pxl, 1, 0);
		rl2_set_pixel_sample_uint16 (pxl, 2, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample)
	    {
	    case RL2_SAMPLE_INT8:
		rl2_set_pixel_sample_int8 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 0);
		break;
	    case RL2_SAMPLE_INT16:
		rl2_set_pixel_sample_int16 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		break;
	    case RL2_SAMPLE_INT32:
		rl2_set_pixel_sample_int32 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT32:
		rl2_set_pixel_sample_uint32 (pxl, 0);
		break;
	    case RL2_SAMPLE_FLOAT:
		rl2_set_pixel_sample_float (pxl, 0.0);
		break;
	    case RL2_SAMPLE_DOUBLE:
		rl2_set_pixel_sample_double (pxl, 0.0);
		break;
	    };
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (sample)
	    {
	    case RL2_SAMPLE_UINT8:
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint8 (pxl, nb, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint16 (pxl, nb, 0);
		break;
	    };
	  break;
      };
    return pxl;
}

RL2_PRIVATE WmsRetryListPtr
alloc_retry_list ()
{
/* initializing an empty WMS retry-list */
    WmsRetryListPtr lst = malloc (sizeof (WmsRetryList));
    lst->first = NULL;
    lst->last = NULL;
    return lst;
}

RL2_PRIVATE void
free_retry_list (WmsRetryListPtr lst)
{
/* memory cleanup - destroying a list of WMS retry requests */
    WmsRetryItemPtr p;
    WmsRetryItemPtr pn;
    if (lst == NULL)
	return;
    p = lst->first;
    while (p != NULL)
      {
	  pn = p->next;
	  free (p);
	  p = pn;
      }
    free (lst);
}

RL2_PRIVATE void
add_retry (WmsRetryListPtr lst, double minx, double miny, double maxx,
	   double maxy)
{
/* inserting a WMS retry request into the list */
    WmsRetryItemPtr p;
    if (lst == NULL)
	return;
    p = malloc (sizeof (WmsRetryItem));
    p->done = 0;
    p->count = 0;
    p->minx = minx;
    p->miny = miny;
    p->maxx = maxx;
    p->maxy = maxy;
    p->next = NULL;
    if (lst->first == NULL)
	lst->first = p;
    if (lst->last != NULL)
	lst->last->next = p;
    lst->last = p;
}

RL2_PRIVATE gaiaGeomCollPtr
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

RL2_PRIVATE int
do_insert_wms_tile (sqlite3 * handle, unsigned char *blob_odd, int blob_odd_sz,
		    unsigned char *blob_even, int blob_even_sz,
		    sqlite3_int64 section_id, int srid, double res_x,
		    double res_y, unsigned short tile_w, unsigned short tile_h,
		    double miny, double maxx, double tile_minx,
		    double tile_miny, double tile_maxx, double tile_maxy,
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
    tile_maxx = tile_minx + ((double) tile_w * res_x);
    if (tile_maxx > maxx)
	tile_maxx = maxx;
    tile_miny = tile_maxy - ((double) tile_h * res_y);
    if (tile_miny < miny)
	tile_miny = miny;
    geom = build_extent (srid, tile_minx, tile_miny, tile_maxx, tile_maxy);
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

RL2_PRIVATE int
do_insert_levels (sqlite3 * handle, unsigned char sample_type, double res_x,
		  double res_y, sqlite3_stmt * stmt_levl)
{
/* INSERTing the base-levels */
    int ret;
    sqlite3_reset (stmt_levl);
    sqlite3_clear_bindings (stmt_levl);
    sqlite3_bind_double (stmt_levl, 1, res_x);
    sqlite3_bind_double (stmt_levl, 2, res_y);
    if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	|| sample_type == RL2_SAMPLE_4_BIT)
      {
	  sqlite3_bind_null (stmt_levl, 3);
	  sqlite3_bind_null (stmt_levl, 4);
	  sqlite3_bind_null (stmt_levl, 5);
	  sqlite3_bind_null (stmt_levl, 6);
	  sqlite3_bind_null (stmt_levl, 7);
	  sqlite3_bind_null (stmt_levl, 8);
      }
    else
      {
	  sqlite3_bind_double (stmt_levl, 3, res_x * 2.0);
	  sqlite3_bind_double (stmt_levl, 4, res_y * 2.0);
	  sqlite3_bind_double (stmt_levl, 5, res_x * 4.0);
	  sqlite3_bind_double (stmt_levl, 6, res_y * 4.0);
	  sqlite3_bind_double (stmt_levl, 7, res_x * 8.0);
	  sqlite3_bind_double (stmt_levl, 8, res_y * 8.0);
      }
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

RL2_PRIVATE int
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

RL2_PRIVATE char *
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

RL2_PRIVATE int
do_insert_section (sqlite3 * handle, const char *src_path,
		   const char *section, int srid, unsigned short width,
		   unsigned short height, double minx, double miny,
		   double maxx, double maxy, sqlite3_stmt * stmt_sect,
		   sqlite3_int64 * id)
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
	  if (sect_name != NULL)
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

RL2_PRIVATE rl2RasterPtr
build_wms_tile (rl2CoveragePtr coverage, const unsigned char *rgba_tile)
{
/* building a raster starting from an RGBA buffer */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    const unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    int requires_mask = 0;
    int x;
    int y;

    if (coverage == NULL || rgba_tile == NULL)
	return NULL;

    /* supporting only RGB, GRAYSCALE or MONOCHROME coverages */
    if (cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	pixels_sz = cvg->tileWidth * cvg->tileHeight * 3;
    if (cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	pixels_sz = cvg->tileWidth * cvg->tileHeight;
    if (cvg->pixelType == RL2_PIXEL_MONOCHROME && cvg->nBands == 1)
	pixels_sz = cvg->tileWidth * cvg->tileHeight;
    if (pixels_sz <= 0)
	return NULL;
    mask_size = cvg->tileWidth * cvg->tileHeight;

    /* allocating the raster and mask buffers */
    pixels = malloc (pixels_sz);
    if (pixels == NULL)
	return NULL;
    mask = malloc (mask_size);
    if (mask == NULL)
      {
	  free (pixels);
	  return NULL;
      }

    p_msk = mask;
    for (x = 0; x < mask_size; x++)
      {
	  /* priming a full opaque mask */
	  *p_msk++ = 1;
      }

    /* copying pixels */
    p_in = rgba_tile;
    p_out = pixels;
    p_msk = mask;
    if (cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
      {
	  for (y = 0; y < cvg->tileHeight; y++)
	    {
		for (x = 0; x < cvg->tileWidth; x++)
		  {
		      unsigned char red = *p_in++;
		      unsigned char green = *p_in++;
		      unsigned char blue = *p_in++;
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		  }
	    }
      }
    if (cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
      {
	  for (y = 0; y < cvg->tileHeight; y++)
	    {
		for (x = 0; x < cvg->tileWidth; x++)
		  {
		      unsigned char red = *p_in++;
		      p_in += 3;
		      *p_out++ = red;
		  }
	    }
      }
    if (cvg->pixelType == RL2_PIXEL_MONOCHROME && cvg->nBands == 1)
      {
	  for (y = 0; y < cvg->tileHeight; y++)
	    {
		for (x = 0; x < cvg->tileWidth; x++)
		  {
		      unsigned char red = *p_in++;
		      p_in += 3;
		      if (red == 255)
			  *p_out++ = 0;
		      else
			  *p_out++ = 1;
		  }
	    }
      }

    if (!requires_mask)
      {
	  /* no mask required */
	  free (mask);
	  mask = NULL;
	  mask_size = 0;
      }

    raster =
	rl2_create_raster (cvg->tileWidth, cvg->tileHeight,
			   cvg->sampleType, cvg->pixelType,
			   cvg->nBands, pixels, pixels_sz, NULL, mask,
			   mask_size, NULL);
    if (raster == NULL)
	goto error;
    return raster;

  error:
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (pixels != NULL)
	free (pixels);
    if (mask != NULL)
	free (mask);
    return NULL;
}

RL2_PRIVATE int
insert_wms_tile (InsertWmsPtr ptr, int *first,
		 rl2RasterStatisticsPtr * section_stats,
		 sqlite3_int64 * section_id)
{
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    rl2RasterPtr raster = NULL;

    if (*first)
      {
	  /* INSERTing the section */
	  *first = 0;
	  if (!do_insert_section
	      (ptr->sqlite, "WMS Service", ptr->sect_name, ptr->srid,
	       ptr->width, ptr->height, ptr->minx, ptr->miny, ptr->maxx,
	       ptr->maxy, ptr->stmt_sect, section_id))
	      goto error;
	  *section_stats =
	      rl2_create_raster_statistics (ptr->sample_type, ptr->num_bands);
	  if (*section_stats == NULL)
	      goto error;
	  /* INSERTing the base-levels */
	  if (!do_insert_levels
	      (ptr->sqlite, RL2_SAMPLE_UNKNOWN, ptr->horz_res, ptr->vert_res,
	       ptr->stmt_levl))
	      goto error;
      }

    /* building the raster tile */
    raster = build_wms_tile (ptr->coverage, ptr->rgba_tile);
    if (raster == NULL)
      {
	  fprintf (stderr, "ERROR: unable to get a WMS tile\n");
	  goto error;
      }
    if (rl2_raster_encode
	(raster, ptr->compression, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 100, 1) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to encode a WMS tile\n");
	  goto error;
      }

    /* INSERTing the tile */
    tile_minx = ptr->x;
    tile_maxx = tile_minx + ptr->tilew;
    tile_maxy = ptr->y;
    tile_miny = tile_maxy - ptr->tileh;
    if (!do_insert_wms_tile
	(ptr->sqlite, blob_odd, blob_odd_sz, blob_even, blob_even_sz,
	 *section_id, ptr->srid, ptr->horz_res, ptr->vert_res, ptr->tile_width,
	 ptr->tile_height, ptr->minx, ptr->maxy, tile_minx, tile_miny,
	 tile_maxx, tile_maxy, NULL, ptr->no_data, ptr->stmt_tils,
	 ptr->stmt_data, *section_stats))
	goto error;
    blob_odd = NULL;
    blob_even = NULL;
    rl2_destroy_raster (raster);
    free (ptr->rgba_tile);
    ptr->rgba_tile = NULL;
    return 1;
  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    free (ptr->rgba_tile);
    ptr->rgba_tile = NULL;
    return 0;
}

RL2_PRIVATE int
is_point (gaiaGeomCollPtr geom)
{
/* checking if the Geom is a simple Point */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    pt = geom->FirstPoint;
    while (pt != NULL)
      {
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg != NULL)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 1 && lns == 0 && pgs == 0)
	return 1;
    return 0;
}

RL2_PRIVATE ResolutionsListPtr
alloc_resolutions_list ()
{
/* allocating an empty list */
    ResolutionsListPtr list = malloc (sizeof (ResolutionsList));
    if (list == NULL)
	return NULL;
    list->first = NULL;
    list->last = NULL;
    return list;
}

RL2_PRIVATE void
destroy_resolutions_list (ResolutionsListPtr list)
{
/* memory cleanup - destroying a list */
    ResolutionLevelPtr res;
    ResolutionLevelPtr resn;
    if (list == NULL)
	return;
    res = list->first;
    while (res != NULL)
      {
	  resn = res->next;
	  free (res);
	  res = resn;
      }
    free (list);
}

RL2_PRIVATE void
add_base_resolution (ResolutionsListPtr list, int level, int scale,
		     double x_res, double y_res)
{
/* inserting a base resolution into the list */
    ResolutionLevelPtr res;
    if (list == NULL)
	return;

    res = list->first;
    while (res != NULL)
      {
	  if (res->x_resolution == x_res && res->y_resolution == y_res)
	    {
		/* already defined: skipping */
		return;
	    }
	  res = res->next;
      }

/* inserting */
    res = malloc (sizeof (ResolutionLevel));
    res->level = level;
    res->scale = scale;
    res->x_resolution = x_res;
    res->y_resolution = y_res;
    res->prev = list->last;
    res->next = NULL;
    if (list->first == NULL)
	list->first = res;
    if (list->last != NULL)
	list->last->next = res;
    list->last = res;
}

RL2_PRIVATE int
find_best_resolution_level (sqlite3 * handle, const char *coverage,
			    double x_res, double y_res, int *level_id,
			    int *scale, int *real_scale, double *xx_res,
			    double *yy_res)
{
/* attempting to identify the optimal resolution level */
    int ret;
    int found = 0;
    int z_level;
    int z_scale;
    int z_real;
    double z_x_res;
    double z_y_res;
    char *xcoverage;
    char *xxcoverage;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    ResolutionsListPtr list = NULL;
    ResolutionLevelPtr res;

    if (coverage == NULL)
	return 0;

    xcoverage = sqlite3_mprintf ("%s_levels", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    sql =
	sqlite3_mprintf
	("SELECT pyramid_level, x_resolution_1_8, y_resolution_1_8, "
	 "x_resolution_1_4, y_resolution_1_4, x_resolution_1_2, y_resolution_1_2, "
	 "x_resolution_1_1, y_resolution_1_1 FROM \"%s\" "
	 "ORDER BY pyramid_level DESC", xxcoverage);
    free (xxcoverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_free (sql);

    list = alloc_resolutions_list ();
    if (list == NULL)
	goto error;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int lvl = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 1);
		      z_y_res = sqlite3_column_double (stmt, 2);
		      add_base_resolution (list, lvl, RL2_SCALE_8, z_x_res,
					   z_y_res);
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 4) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 3);
		      z_y_res = sqlite3_column_double (stmt, 4);
		      add_base_resolution (list, lvl, RL2_SCALE_4, z_x_res,
					   z_y_res);
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 5);
		      z_y_res = sqlite3_column_double (stmt, 6);
		      add_base_resolution (list, lvl, RL2_SCALE_2, z_x_res,
					   z_y_res);
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 7);
		      z_y_res = sqlite3_column_double (stmt, 8);
		      add_base_resolution (list, lvl, RL2_SCALE_1, z_x_res,
					   z_y_res);
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

/* adjusting real scale factors */
    z_real = 1;
    res = list->last;
    while (res != NULL)
      {
	  res->real_scale = z_real;
	  z_real *= 2;
	  res = res->prev;
      }
/* retrieving the best resolution level */
    found = 0;
    res = list->last;
    while (res != NULL)
      {
	  if (res->x_resolution <= x_res && res->y_resolution <= y_res)
	    {
		found = 1;
		z_level = res->level;
		z_scale = res->scale;
		z_real = res->real_scale;
		z_x_res = res->x_resolution;
		z_y_res = res->y_resolution;
	    }
	  res = res->prev;
      }
    if (found)
      {
	  *level_id = z_level;
	  *scale = z_scale;
	  *real_scale = z_real;
	  *xx_res = z_x_res;
	  *yy_res = z_y_res;
      }
    else if (list->last != NULL)
      {
	  res = list->last;
	  *level_id = res->level;
	  *scale = res->scale;
	  *xx_res = res->x_resolution;
	  *yy_res = res->y_resolution;
      }
    else
	goto error;
    destroy_resolutions_list (list);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (list != NULL)
	destroy_resolutions_list (list);
    return 0;
}

RL2_PRIVATE unsigned char
get_palette_format (rl2PrivPalettePtr plt)
{
/* testing for a Grayscale or RGB palette */
    int is_gray = 0;
    int i;
    for (i = 0; i < plt->nEntries; i++)
      {
	  rl2PrivPaletteEntryPtr entry = plt->entries + i;
	  if (entry->red == entry->green && entry->red == entry->blue)
	      is_gray++;
      }
    if (is_gray == plt->nEntries)
	return RL2_PIXEL_GRAYSCALE;
    else
	return RL2_PIXEL_RGB;
}
