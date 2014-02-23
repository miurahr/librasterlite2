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
#define ARG_MODE_DE_PYRAMIDIZE	7
#define ARG_MODE_LIST		8
#define ARG_MODE_CATALOG	9
#define ARG_MODE_MAP		10

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
#define ARG_MINX		30
#define ARG_MINY		31
#define ARG_MAXX		32
#define ARG_MAXY		33
#define ARG_CX			34
#define ARG_CY			35
#define ARG_NO_DATA		36

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

struct pyramid_virtual
{
    double h_res;
    double v_res;
    struct pyramid_virtual *next;
};

struct pyramid_level
{
    int level_id;
    double h_res;
    double v_res;
    double tiles_count;
    double blob_bytes;
    struct pyramid_virtual *first;
    struct pyramid_virtual *last;
    struct pyramid_level *next;
};

struct pyramid_infos
{
    struct pyramid_level *first;
    struct pyramid_level *last;
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

static rl2PixelPtr
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

static int
exec_create (sqlite3 * handle, const char *coverage,
	     unsigned char sample, unsigned char pixel, unsigned char num_bands,
	     unsigned char compression, int quality, unsigned short tile_width,
	     unsigned short tile_height, int srid, double x_res, double y_res,
	     rl2PixelPtr no_data)
{
/* performing CREATE */
    rl2PalettePtr palette = NULL;

    if (no_data == NULL)
      {
	  /* creating a default NO-DATA value */
	  no_data = default_nodata (sample, pixel, num_bands);
      }
    if (pixel == RL2_PIXEL_PALETTE)
      {
/* creating a default PALETTE */
	  palette = rl2_create_palette (1);
	  rl2_set_palette_color (palette, 0, 255, 255, 255);
      }

    if (rl2_create_dbms_coverage
	(handle, coverage, sample, pixel, num_bands, compression, quality,
	 tile_width, tile_height, srid, x_res, y_res, no_data,
	 palette) != RL2_OK)
	return 0;

    fprintf (stderr, "\rRaster Coverage \"%s\" successfully created\n",
	     coverage);
    return 1;
}

static int
exec_import (sqlite3 * handle, const char *src_path, const char *dir_path,
	     const char *file_ext, const char *coverage,
	     int worldfile, int force_srid, int pyramidize)
{
/* performing IMPORT */
    int ret;
    rl2CoveragePtr cvg = NULL;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (src_path != NULL)
	ret =
	    rl2_load_raster_into_dbms (handle, src_path, cvg, worldfile,
				       force_srid, pyramidize);
    else
	ret =
	    rl2_load_mrasters_into_dbms (handle, dir_path, file_ext, cvg,
					 worldfile, force_srid, pyramidize);
    rl2_destroy_coverage (cvg);
    if (ret == RL2_OK)
	return 1;
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
exec_export (sqlite3 * handle, const char *dst_path, const char *coverage,
	     double x_res, double y_res, double minx, double miny, double maxx,
	     double maxy, unsigned short width, unsigned short height)
{
/* performing EXPORT */
    rl2CoveragePtr cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	return 0;
    if (is_ascii_grid (dst_path))
      {
	  /* export an ASCII Grid */
	  if (rl2_export_ascii_grid_from_dbms
	      (handle, dst_path, cvg, x_res, minx, miny, maxx, maxy, width,
	       height, 0, 4) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* export a GeoTIFF */
	  if (rl2_export_geotiff_from_dbms
	      (handle, dst_path, cvg, x_res, y_res, minx, miny, maxx, maxy,
	       width, height, RL2_COMPRESSION_NONE, 256, 0) != RL2_OK)
	      goto error;
      }
    rl2_destroy_coverage (cvg);
    return 1;
  error:
    rl2_destroy_coverage (cvg);
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
		fprintf (stderr, "%1.2f %1.2f\n", cx, cy);
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
			    fprintf (stderr, "\trd=%lld\n", tile_id);
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
						   blob_even_sz, NULL);
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
    fprintf (stderr, "Encode %d/%d\n", row, col);
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
    if (section == NULL)
	ret = rl2_build_all_section_pyramids (handle, coverage, force_pyramid);
    else
	ret =
	    rl2_build_section_pyramid (handle, coverage, section,
				       force_pyramid);
    if (ret == RL2_OK)
	return 1;
    return 0;
}

static int
exec_de_pyramidize (sqlite3 * handle, const char *coverage, const char *section)
{
/* deleting Pyramid levels */
    int ret;
    if (section == NULL)
	ret = rl2_delete_all_pyramids (handle, coverage);
    else
	ret = rl2_delete_section_pyramid (handle, coverage, section);
    if (ret == RL2_OK)
	return 1;
    return 0;
}

static void
add_pyramid_level (struct pyramid_infos *pyramid, int level_id, double h_res,
		   double v_res)
{
/* inserting a physical Pyramid Level into the list */
    struct pyramid_level *lvl = malloc (sizeof (struct pyramid_level));
    lvl->level_id = level_id;
    lvl->h_res = h_res;
    lvl->v_res = v_res;
    lvl->tiles_count = 0;
    lvl->blob_bytes = 0;
    lvl->first = NULL;
    lvl->last = NULL;
    lvl->next = NULL;
    if (pyramid->first == NULL)
	pyramid->first = lvl;
    if (pyramid->last != NULL)
	pyramid->last->next = lvl;
    pyramid->last = lvl;
}

static void
set_pyramid_level (struct pyramid_infos *pyramid, int level_id,
		   sqlite3_int64 count, sqlite3_int64 size_odd,
		   sqlite3_int64 size_even)
{
/* setting up values for a physical Pyramid Level */
    struct pyramid_level *lvl = pyramid->first;
    while (lvl != NULL)
      {
	  if (lvl->level_id == level_id)
	    {
		lvl->tiles_count = count;
		lvl->blob_bytes = size_odd;
		lvl->blob_bytes += size_even;
		return;
	    }
	  lvl = lvl->next;
      }
}

static void
add_pyramid_virtual (struct pyramid_infos *pyramid, int level_id, double h_res,
		     double v_res)
{
/* inserting a virtual Pyramid Level into the list */
    struct pyramid_level *lvl = pyramid->first;
    while (lvl != NULL)
      {
	  if (lvl->level_id == level_id)
	    {
		struct pyramid_virtual *vrt =
		    malloc (sizeof (struct pyramid_virtual));
		vrt->h_res = h_res;
		vrt->v_res = v_res;
		vrt->next = NULL;
		if (lvl->first == NULL)
		    lvl->first = vrt;
		if (lvl->last != NULL)
		    lvl->last->next = vrt;
		lvl->last = vrt;
		return;
	    }
	  lvl = lvl->next;
      }
}

static void
free_pyramid_level (struct pyramid_level *level)
{
/* memory cleanup - destroying a Pyramid Level object */
    struct pyramid_virtual *lev;
    struct pyramid_virtual *n_lev;
    if (level == NULL)
	return;
    lev = level->first;
    while (lev)
      {
	  n_lev = lev->next;
	  free (lev);
	  lev = n_lev;
      }
    free (level);
}

static void
free_pyramid_infos (struct pyramid_infos *pyramid)
{
/* memory cleanup - destroying a Pyramid Infos object */
    struct pyramid_level *lev;
    struct pyramid_level *n_lev;
    if (pyramid == NULL)
	return;
    lev = pyramid->first;
    while (lev)
      {
	  n_lev = lev->next;
	  free_pyramid_level (lev);
	  lev = n_lev;
      }
    free (pyramid);
}

static struct pyramid_infos *
get_pyramid_infos (sqlite3 * handle, const char *coverage)
{
    char *sql;
    char *xtable;
    char *xxtable;
    char *xtable2;
    char *xxtable2;
    int ret;
    sqlite3_stmt *stmt = NULL;
    struct pyramid_infos *pyramid = malloc (sizeof (struct pyramid_infos));
    pyramid->first = NULL;
    pyramid->last = NULL;

    xtable = sqlite3_mprintf ("%s_levels", coverage);
    xxtable = gaiaDoubleQuotedSql (xtable);
    sqlite3_free (xtable);
    sql =
	sqlite3_mprintf
	("SELECT pyramid_level, x_resolution_1_1, y_resolution_1_1, "
	 "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, y_resolution_1_4, "
	 "x_resolution_1_8, y_resolution_1_8 "
	 "FROM \"%s\" ORDER BY pyramid_level", xxtable);
    free (xxtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("Levels SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  /* querying the Pyramid Levels */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int level_id = sqlite3_column_int (stmt, 0);
		double h_res = sqlite3_column_double (stmt, 1);
		double v_res = sqlite3_column_double (stmt, 2);
		add_pyramid_level (pyramid, level_id, h_res, v_res);
		if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 4) == SQLITE_FLOAT)
		  {
		      h_res = sqlite3_column_double (stmt, 3);
		      v_res = sqlite3_column_double (stmt, 4);
		      add_pyramid_virtual (pyramid, level_id, h_res, v_res);
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
		  {
		      h_res = sqlite3_column_double (stmt, 5);
		      v_res = sqlite3_column_double (stmt, 6);
		      add_pyramid_virtual (pyramid, level_id, h_res, v_res);
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      h_res = sqlite3_column_double (stmt, 7);
		      v_res = sqlite3_column_double (stmt, 8);
		      add_pyramid_virtual (pyramid, level_id, h_res, v_res);
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "Levels sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (pyramid->first == NULL)
	goto error;

    xtable = sqlite3_mprintf ("%s_tiles", coverage);
    xxtable = gaiaDoubleQuotedSql (xtable);
    sqlite3_free (xtable);
    xtable2 = sqlite3_mprintf ("%s_tile_data", coverage);
    xxtable2 = gaiaDoubleQuotedSql (xtable2);
    sqlite3_free (xtable2);
    sql =
	sqlite3_mprintf
	("SELECT t.pyramid_level, Count(*), Sum(Length(d.tile_data_odd)), "
	 "Sum(Length(d.tile_data_even)) FROM \"%s\" AS t "
	 "JOIN \"%s\" AS d ON (d.tile_id = t.tile_id) "
	 "GROUP BY t.pyramid_level", xxtable, xxtable2);
    free (xxtable);
    free (xxtable2);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("Tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  /* querying the Pyramid Tiles */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int level_id = sqlite3_column_int (stmt, 0);
		sqlite3_int64 count = sqlite3_column_int (stmt, 1);
		sqlite3_int64 size_odd = sqlite3_column_int (stmt, 2);
		sqlite3_int64 size_even = 0;
		if (sqlite3_column_type (stmt, 3) != SQLITE_NULL)
		    size_even = sqlite3_column_int (stmt, 3);
		set_pyramid_level (pyramid, level_id, count, size_odd,
				   size_even);
	    }
	  else
	    {
		fprintf (stderr,
			 "Tiles sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    return pyramid;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (pyramid != NULL)
	free_pyramid_infos (pyramid);
    return NULL;
}

static int
exec_catalog (sqlite3 * handle)
{
/* Rasterlite-2 datasources Catalog */
    const char *sql;
    int ret;
    int count = 0;
    char *hres;
    char *vres;
    sqlite3_stmt *stmt = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PalettePtr palette = NULL;
    rl2RasterStatisticsPtr statistics = NULL;
    struct pyramid_infos *pyramid = NULL;

    sql =
	"SELECT coverage_name, title, abstract, sample_type, pixel_type, "
	"num_bands, compression, quality, tile_width, tile_height, "
	"horz_resolution, vert_resolution, srid, auth_name, auth_srid, "
	"ref_sys_name, extent_minx, extent_miny, extent_maxx, extent_maxy, "
	"nodata_pixel, palette, statistics "
	"FROM raster_coverages_ref_sys ORDER BY coverage_name";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto stop;

    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = (const char *)sqlite3_column_text (stmt, 0);
		const char *title = (const char *)sqlite3_column_text (stmt, 1);
		const char *abstract = (const char *)sqlite3_column_text (stmt, 2);
		const char *sample = (const char *)sqlite3_column_text (stmt, 3);
		const char *pixel = (const char *)sqlite3_column_text (stmt, 4);
		int bands = sqlite3_column_int (stmt, 5);
		const char *compression = (const char *)sqlite3_column_text (stmt, 6);
		int quality = sqlite3_column_int (stmt, 7);
		int tileW = sqlite3_column_int (stmt, 8);
		int tileH = sqlite3_column_int (stmt, 9);
		double x_res = sqlite3_column_double (stmt, 10);
		double y_res = sqlite3_column_double (stmt, 11);
		int srid = sqlite3_column_int (stmt, 12);
		const char *authName = (const char *)sqlite3_column_text (stmt, 13);
		int authSrid = sqlite3_column_int (stmt, 14);
		const char *crsName = (const char *)sqlite3_column_text (stmt, 15);
		char *minx = NULL;
		char *miny = NULL;
		char *maxx = NULL;
		char *maxy = NULL;
		int horz = -1;
		int vert = -1;
		double mnx = DBL_MAX;
		double mny = DBL_MAX;
		double mxx = DBL_MAX;
		double mxy = DBL_MAX;
		const unsigned char *blob;
		int blob_sz;
		if (sqlite3_column_type (stmt, 16) == SQLITE_FLOAT)
		  {
		      mnx = sqlite3_column_double (stmt, 16);
		      minx = formatFloat (mnx);
		  }
		if (sqlite3_column_type (stmt, 17) == SQLITE_FLOAT)
		  {
		      mny = sqlite3_column_double (stmt, 17);
		      miny = formatFloat (mny);
		  }
		if (sqlite3_column_type (stmt, 18) == SQLITE_FLOAT)
		  {
		      mxx = sqlite3_column_double (stmt, 18);
		      maxx = formatFloat (mxx);
		  }
		if (sqlite3_column_type (stmt, 19) == SQLITE_FLOAT)
		  {
		      mxy = sqlite3_column_double (stmt, 19);
		      maxy = formatFloat (mxy);
		  }
		if (mnx != DBL_MAX && mny != DBL_MAX && mxx != DBL_MAX
		    && mxy != DBL_MAX)
		  {
		      double ext_x = mxx - mnx;
		      double ext_y = mxy - mny;
		      horz = (int) (ext_x / x_res);
		      vert = (int) (ext_y / y_res);
		  }
		if (sqlite3_column_type (stmt, 20) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt, 20);
		      blob_sz = sqlite3_column_bytes (stmt, 20);
		      no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
		  }
		if (sqlite3_column_type (stmt, 21) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt, 21);
		      blob_sz = sqlite3_column_bytes (stmt, 21);
		      palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
		if (sqlite3_column_type (stmt, 22) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt, 22);
		      blob_sz = sqlite3_column_bytes (stmt, 22);
		      statistics =
			  rl2_deserialize_dbms_raster_statistics (blob,
								  blob_sz);
		  }
		pyramid = get_pyramid_infos (handle, name);
		count++;
		printf
		    ("===============================================================================\n");
		printf ("             Coverage: %s\n", name);
		printf ("                Title: %s\n", title);
		printf ("             Abstract: %s\n", abstract);
		printf
		    ("-------------------------------------------------------------------------------\n");
		printf ("          Sample Type: %s\n", sample);
		printf ("           Pixel Type: %s\n", pixel);
		printf ("      Number of Bands: %d\n", bands);
		if (strcmp (compression, "NONE") == 0)
		    printf ("          Compression: NONE (uncompressed)\n");
		else if (strcmp (compression, "DEFLATE") == 0)
		    printf ("          Compression: DEFLATE (zip, lossless)\n");
		else if (strcmp (compression, "LZMA") == 0)
		    printf ("          Compression: LZMA (7-zip, lossless)\n");
		else if (strcmp (compression, "PNG") == 0)
		    printf ("          Compression: PNG, lossless\n");
		else if (strcmp (compression, "JPEG") == 0)
		    printf ("          Compression: JPEG (lossy)\n");
		else if (strcmp (compression, "WEBP") == 0)
		    printf ("          Compression: WEBP (lossy)\n");
		else if (strcmp (compression, "LL_WEBP") == 0)
		    printf ("          Compression: WEBP, lossless\n");
		else if (strcmp (compression, "FAX4") == 0)
		    printf ("          Compression: CCITT-FAX4 lossless\n");
		if (strcmp (compression, "JPEG") == 0
		    || strcmp (compression, "WEBP") == 0)
		    printf ("  Compression Quality: %d\n", quality);
		printf ("   Tile Size (pixels): %d x %d\n", tileW, tileH);
		hres = formatFloat (x_res);
		vres = formatFloat (y_res);
		printf ("Pixel base resolution: X=%s Y=%s\n", hres, vres);
		printf
		    ("-------------------------------------------------------------------------------\n");
		sqlite3_free (hres);
		sqlite3_free (vres);
		if (horz >= 0 && vert >= 0)
		    printf ("Overall Size (pixels): %d x %d\n", horz, vert);
		else
		    printf ("Overall Size (pixels): *** undefined ***\n");
		printf ("                 Srid: %d (%s,%d) %s\n", srid,
			authName, authSrid, crsName);
		if (minx == NULL || miny == NULL)
		    printf ("      LowerLeftCorner: *** undefined ***\n");
		else
		    printf ("      LowerLeftCorner: X=%s Y=%s\n", minx, miny);
		if (minx == NULL || miny == NULL)
		    printf ("     UpperRightCorner: *** undefined ***\n");
		else
		    printf ("     UpperRightCorner: X=%s Y=%s\n", minx, miny);
		printf
		    ("-------------------------------------------------------------------------------\n");
		if (no_data == NULL)
		    printf ("        NO-DATA Pixel: *** undefined ***\n");
		else
		  {
		      unsigned char i;
		      unsigned char sample_type;
		      unsigned char pixel_type;
		      unsigned char num_bands;
		      rl2_get_pixel_type (no_data, &sample_type, &pixel_type,
					  &num_bands);
		      printf ("        NO-DATA Pixel: ");
		      for (i = 0; i < num_bands; i++)
			{
			    char *val;
			    char val_int8;
			    short val_int16;
			    int val_int32;
			    unsigned char val_uint8;
			    unsigned short val_uint16;
			    unsigned int val_uint32;
			    float val_float;
			    double val_double;
			    if (i > 0)
				printf (", ");
			    switch (sample_type)
			      {
			      case RL2_SAMPLE_1_BIT:
				  rl2_get_pixel_sample_1bit (no_data,
							     &val_uint8);
				  printf ("%u", val_uint8);
				  break;
			      case RL2_SAMPLE_2_BIT:
				  rl2_get_pixel_sample_2bit (no_data,
							     &val_uint8);
				  printf ("%u", val_uint8);
				  break;
			      case RL2_SAMPLE_4_BIT:
				  rl2_get_pixel_sample_4bit (no_data,
							     &val_uint8);
				  printf ("%u", val_uint8);
				  break;
			      case RL2_SAMPLE_INT8:
				  rl2_get_pixel_sample_int8 (no_data,
							     &val_int8);
				  printf ("%d", val_int8);
				  break;
			      case RL2_SAMPLE_INT16:
				  rl2_get_pixel_sample_int16 (no_data,
							      &val_int16);
				  printf ("%d", val_int16);
				  break;
			      case RL2_SAMPLE_INT32:
				  rl2_get_pixel_sample_int32 (no_data,
							      &val_int32);
				  printf ("%d", val_int32);
				  break;
			      case RL2_SAMPLE_UINT8:
				  rl2_get_pixel_sample_uint8 (no_data, i,
							      &val_uint8);
				  printf ("%u", val_uint8);
				  break;
			      case RL2_SAMPLE_UINT16:
				  rl2_get_pixel_sample_uint16 (no_data, i,
							       &val_uint16);
				  printf ("%u", val_uint16);
				  break;
			      case RL2_SAMPLE_UINT32:
				  rl2_get_pixel_sample_uint32 (no_data,
							       &val_uint32);
				  printf ("%u", val_uint32);
				  break;
			      case RL2_SAMPLE_FLOAT:
				  rl2_get_pixel_sample_float (no_data,
							      &val_float);
				  val = formatFloat (val_float);
				  printf ("%s", val);
				  sqlite3_free (val);
				  break;
			      case RL2_SAMPLE_DOUBLE:
				  rl2_get_pixel_sample_double (no_data,
							       &val_double);
				  val = formatFloat (val_double);
				  printf ("%s", val);
				  sqlite3_free (val);
				  break;
			      }
			}
		      printf ("\n");

		  }
		if (palette != NULL)
		  {
		      /* printing an eventual Palette */
		      unsigned char i;
		      unsigned short num_entries;
		      unsigned char *red = NULL;
		      unsigned char *green = NULL;
		      unsigned char *blue = NULL;
		      rl2_get_palette_colors (palette, &num_entries, &red,
					      &green, &blue);
		      for (i = 0; i < num_entries; i++)
			{
			    if (i == 0)
				printf
				    ("    Color Palette %3d: #%02x%02x%02x\n",
				     i, *(red + i), *(green + i), *(blue + i));
			    else
				printf
				    ("                  %3d: #%02x%02x%02x\n",
				     i, *(red + i), *(green + i), *(blue + i));
			}
		  }
		if (statistics != NULL)
		  {
		      /* printing the Statistics summary (if any) */
		      unsigned char i;
		      double no_data;
		      double count;
		      unsigned char sample_type;
		      unsigned char num_bands;
		      rl2_get_raster_statistics_summary (statistics, &no_data,
							 &count, &sample_type,
							 &num_bands);
		      printf
			  ("-------------------------------------------------------------------------------\n");
		      printf (" NO-DATA Pixels Count: %1.0f\n", no_data);
		      printf ("   Valid Pixels Count: %1.0f\n", count);
		      for (i = 0; i < num_bands; i++)
			{
			    /* band summary */
			    double min;
			    double max;
			    double mean;
			    double var;
			    double stddev;
			    char *val;
			    printf
				("  =========== Band #%u Summary ==========\n",
				 i);
			    rl2_get_band_statistics (statistics, i, &min, &max,
						     &mean, &var, &stddev);
			    if (sample_type == RL2_SAMPLE_FLOAT
				|| sample_type == RL2_SAMPLE_DOUBLE)
			      {
				  val = formatFloat (min);
				  printf ("            Min Value: %s\n", val);
				  sqlite3_free (val);
				  val = formatFloat (max);
				  printf ("            Max Value: %s\n", val);
				  sqlite3_free (val);
			      }
			    else
			      {
				  printf ("            Min Value: %1.0f\n",
					  min);
				  printf ("            Max Value: %1.0f\n",
					  max);
			      }
			    val = formatFloat (mean);
			    printf ("           Mean Value: %s\n", val);
			    sqlite3_free (val);
			    val = formatFloat (var);
			    printf ("             Variance: %s\n", val);
			    sqlite3_free (val);
			    val = formatFloat (stddev);
			    printf ("   Standard Deviation: %s\n", val);
			    sqlite3_free (val);
			}
		  }
		if (pyramid != NULL)
		  {
		      /* printing Pyramid Level Infos */
		      struct pyramid_level *lvl;
		      struct pyramid_virtual *vrt;
		      printf
			  ("-------------------------------------------------------------------------------\n");
		      lvl = pyramid->first;
		      while (lvl != NULL)
			{
			    printf
				("   ===  Pyramid Level: %d  ==========================\n",
				 lvl->level_id);
			    hres = formatFloat (lvl->h_res);
			    vres = formatFloat (lvl->v_res);
			    printf ("  Physical resolution: X=%s Y=%s\n", hres,
				    vres);
			    sqlite3_free (hres);
			    sqlite3_free (vres);
			    vrt = lvl->first;
			    while (vrt != NULL)
			      {
				  hres = formatFloat (vrt->h_res);
				  vres = formatFloat (vrt->v_res);
				  printf ("   Virtual resolution: X=%s Y=%s\n",
					  hres, vres);
				  sqlite3_free (hres);
				  sqlite3_free (vres);
				  vrt = vrt->next;
			      }
			    printf ("          Tiles Count: %1.0f\n",
				    lvl->tiles_count);
			    printf ("           BLOB Bytes: %1.0f\n",
				    lvl->blob_bytes);
			    lvl = lvl->next;
			}
		  }
		if (minx != NULL)
		    sqlite3_free (minx);
		if (miny != NULL)
		    sqlite3_free (miny);
		if (maxx != NULL)
		    sqlite3_free (maxx);
		if (maxy != NULL)
		    sqlite3_free (maxy);
		if (no_data != NULL)
		    rl2_destroy_pixel (no_data);
		if (palette != NULL)
		    rl2_destroy_palette (palette);
		if (statistics != NULL)
		    rl2_destroy_raster_statistics (statistics);
		no_data = NULL;
		palette = NULL;
		statistics = NULL;
		if (pyramid != NULL)
		    free_pyramid_infos (pyramid);
		pyramid = NULL;
		printf
		    ("===============================================================================\n\n\n");
	    }
	  else
	    {
		fprintf (stderr,
			 "CATALOG; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto stop;
	    }
      }

    sqlite3_finalize (stmt);
    if (count == 0)
	printf ("no Rasterlite-2 datasources found\n");
    return 1;

  stop:
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (statistics != NULL)
	rl2_destroy_raster_statistics (statistics);
    if (pyramid != NULL)
	free_pyramid_infos (pyramid);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    printf ("no Rasterlite-2 datasources found\n");
    return 0;
}

static int
exec_list (sqlite3 * handle, const char *coverage, const char *section)
{
/* Rasterlite-2 datasource Sections list */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *dumb1;
    char *dumb2;
    char *xsections;
    char *xxsections;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    rl2RasterStatisticsPtr statistics = NULL;
    int last_id;
    int first;

    if (section == NULL)
      {
	  /* all sections */
	  xsections = sqlite3_mprintf ("%s_sections", coverage);
	  xxsections = gaiaDoubleQuotedSql (xsections);
	  sqlite3_free (xsections);
	  xtiles = sqlite3_mprintf ("%s_tiles", coverage);
	  xxtiles = gaiaDoubleQuotedSql (xtiles);
	  sqlite3_free (xtiles);
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = gaiaDoubleQuotedSql (xdata);
	  sqlite3_free (xdata);
	  sql =
	      sqlite3_mprintf
	      ("SELECT s.section_id, s.section_name, s.width, s.height, "
	       "s.file_path, MbrMinX(s.geometry), MbrMinY(s.geometry), MbrMaxX(s.geometry), "
	       "MbrMaxY(s.geometry), s.statistics, t.pyramid_level, Count(*), "
	       "Sum(Length(d.tile_data_odd)), Sum(Length(d.tile_data_even)) "
	       "FROM \"%s\" AS s "
	       "JOIN \"%s\" AS t ON (s.section_id = t.section_id) "
	       "JOIN \"%s\" AS d ON (d.tile_id = t.tile_id) "
	       "GROUP BY s.section_id, t.pyramid_level", xxsections, xxtiles,
	       xxdata);
	  free (xxsections);
	  free (xxtiles);
	  free (xxdata);
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
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = gaiaDoubleQuotedSql (xdata);
	  sqlite3_free (xdata);
	  sql =
	      sqlite3_mprintf
	      ("SELECT s.section_id, s.section_name, s.width, s.height, "
	       "s.file_path, MbrMinX(s.geometry), MbrMinY(s.geometry), MbrMaxX(s.geometry), "
	       "MbrMaxY(s.geometry), s.statistics, t.pyramid_level, Count(*), "
	       "Sum(Length(d.tile_data_odd)), Sum(Length(d.tile_data_even)) "
	       "FROM \"%s\" AS s "
	       "JOIN \"%s\" AS t ON (s.section_id = t.section_id) "
	       "JOIN \"%s\" AS d ON (d.tile_id = t.tile_id) "
	       "WHERE s.section_name = %Q "
	       "GROUP BY s.section_id, t.pyramid_level", xxsections,
	       xxtiles, xxdata, section);
	  free (xxsections);
	  free (xxtiles);
	  free (xxdata);
      }
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto stop;

    printf
	("===============================================================================\n");
    printf ("             Coverage: %s\n", coverage);
    printf
	("===============================================================================\n\n");
    last_id = -1;
    first = 1;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int id = sqlite3_column_int (stmt, 0);
		const char *name = (const char *)sqlite3_column_text (stmt, 1);
		int width = sqlite3_column_int (stmt, 2);
		int height = sqlite3_column_int (stmt, 3);
		const char *path = NULL;
		double minx = DBL_MAX;
		double miny = DBL_MAX;
		double maxx = DBL_MAX;
		double maxy = DBL_MAX;
		int level = 0;
		int tiles = 0;
		int blob_bytes = 0;
		if (sqlite3_column_type (stmt, 4) == SQLITE_TEXT)
		    path = (const char *)sqlite3_column_text (stmt, 4);
		if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT)
		    minx = sqlite3_column_double (stmt, 5);
		if (sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
		    miny = sqlite3_column_double (stmt, 6);
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT)
		    maxx = sqlite3_column_double (stmt, 7);
		if (sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		    maxy = sqlite3_column_double (stmt, 8);
		if (sqlite3_column_type (stmt, 9) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 9);
		      int blob_sz = sqlite3_column_bytes (stmt, 9);
		      statistics =
			  rl2_deserialize_dbms_raster_statistics (blob,
								  blob_sz);
		  }
		level = sqlite3_column_int (stmt, 10);
		tiles = sqlite3_column_int (stmt, 11);
		blob_bytes = sqlite3_column_int (stmt, 12);
		if (sqlite3_column_type (stmt, 13) == SQLITE_INTEGER)
		    blob_bytes += sqlite3_column_int (stmt, 13);
		if (last_id != id)
		  {
		      if (!first)
			{
			    printf
				("-------------------------------------------------------------------------------\n");
			    printf ("\n\n\n");
			}
		      else
			  first = 0;
		      printf
			  ("-------------------------------------------------------------------------------\n");
		      printf ("              Section: (%d) %s\n", id, name);
		      printf
			  ("-------------------------------------------------------------------------------\n");
		      printf ("        Size (pixels): %d x %d\n", width,
			      height);
		      if (path == NULL)
			  printf ("           Input Path: *** unknowon ***\n");
		      else
			  printf ("           Input Path: %s\n", path);
		      if (minx == DBL_MAX || miny == DBL_MAX)
			  printf ("     LowerLeft corner: *** unknown ***\n");
		      else
			{
			    dumb1 = formatLong (minx);
			    dumb2 = formatLat (miny);
			    printf ("     LowerLeft corner: X=%s Y=%s\n", dumb1,
				    dumb2);
			    sqlite3_free (dumb1);
			    sqlite3_free (dumb2);
			}
		      if (maxx == DBL_MAX || maxy == DBL_MAX)
			  printf ("    UpperRight corner: *** unknown ***\n");
		      else
			{
			    dumb1 = formatLong (maxx);
			    dumb2 = formatLat (maxy);
			    printf ("    UpperRight corner: X=%s Y=%s\n", dumb1,
				    dumb2);
			    sqlite3_free (dumb1);
			    sqlite3_free (dumb2);
			}
		      if (minx == DBL_MAX || miny == DBL_MAX || maxx == DBL_MAX
			  || maxy == DBL_MAX)
			  printf ("         Center Point: *** unknown ***\n");
		      else
			{
			    dumb1 = formatLong (minx + ((maxx - minx) / 2.0));
			    dumb2 = formatLat (miny + ((maxy - miny) / 2.0));
			    printf ("         Center Point: X=%s Y=%s\n", dumb1,
				    dumb2);
			    sqlite3_free (dumb1);
			    sqlite3_free (dumb2);
			}
		      if (statistics != NULL)
			{
			    /* printing the Statistics summary (if any) */
			    unsigned char i;
			    double no_data;
			    double count;
			    unsigned char sample_type;
			    unsigned char num_bands;
			    rl2_get_raster_statistics_summary (statistics,
							       &no_data, &count,
							       &sample_type,
							       &num_bands);
			    printf
				("-------------------------------------------------------------------------------\n");
			    printf (" NO-DATA Pixels Count: %1.0f\n", no_data);
			    printf ("   Valid Pixels Count: %1.0f\n", count);
			    for (i = 0; i < num_bands; i++)
			      {
				  /* band summary */
				  double min;
				  double max;
				  double mean;
				  double var;
				  double stddev;
				  char *val;
				  printf
				      ("  =========== Band #%u Summary ==========\n",
				       i);
				  rl2_get_band_statistics (statistics, i, &min,
							   &max, &mean, &var,
							   &stddev);
				  if (sample_type == RL2_SAMPLE_FLOAT
				      || sample_type == RL2_SAMPLE_DOUBLE)
				    {
					val = formatFloat (min);
					printf ("            Min Value: %s\n",
						val);
					sqlite3_free (val);
					val = formatFloat (max);
					printf ("            Max Value: %s\n",
						val);
					sqlite3_free (val);
				    }
				  else
				    {
					printf
					    ("            Min Value: %1.0f\n",
					     min);
					printf
					    ("            Max Value: %1.0f\n",
					     max);
				    }
				  val = formatFloat (mean);
				  printf ("           Mean Value: %s\n", val);
				  sqlite3_free (val);
				  val = formatFloat (var);
				  printf ("             Variance: %s\n", val);
				  sqlite3_free (val);
				  val = formatFloat (stddev);
				  printf ("   Standard Deviation: %s\n", val);
				  sqlite3_free (val);
			      }
			    printf
				("-------------------------------------------------------------------------------\n");
			}
		  }
		last_id = id;
		printf ("    ==========  Level: %d  ==========\n", level);
		printf ("          Tiles Count: %d\n", tiles);
		printf ("           BLOB Bytes: %d\n", blob_bytes);
		if (statistics != NULL)
		    rl2_destroy_raster_statistics (statistics);
	    }
	  else
	    {
		fprintf (stderr,
			 "LIST; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto stop;
	    }
      }
    sqlite3_finalize (stmt);
    if (!first)
	printf
	    ("-------------------------------------------------------------------------------\n");
    return 1;

  stop:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (statistics != NULL)
	rl2_destroy_raster_statistics (statistics);
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
    printf ("     SQLite version: %s\n", sqlite3_libversion ());
    printf (" SpatiaLite version: %s\n", spatialite_version ());
    printf ("RasterLite2 version: %s\n\n", rl2_version ());

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
    rl2_init (db_handle, 0);

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
		   double y_res, rl2PixelPtr pxl)
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
	fprintf (stderr, "*** ERROR *** unknown number of Bands\n");
    else
	fprintf (stderr, "      Number of Bands: %d\n", num_bands);
    if (pxl != NULL)
      {
	  unsigned char xsample;
	  unsigned char xpixel;
	  unsigned char xbands;
	  int band_idx;
	  if (rl2_get_pixel_type (pxl, &xsample, &xpixel, &xbands) != RL2_OK)
	      fprintf (stderr, "*** ERROR *** invalid NO-DATA pixel\n");
	  else
	    {
		fprintf (stderr, "        NO-DATA pixel: ");
		for (band_idx = 0; band_idx < xbands; band_idx++)
		  {
		      char int8_value;
		      unsigned char uint8_value;
		      short int16_value;
		      unsigned short uint16_value;
		      int int32_value;
		      unsigned int uint32_value;
		      float flt_value;
		      double dbl_value;
		      char *dummy;
		      switch (xsample)
			{
			case RL2_SAMPLE_1_BIT:
			    rl2_get_pixel_sample_1bit (pxl, &uint8_value);
			    fprintf (stderr, "%u", uint8_value);
			    break;
			case RL2_SAMPLE_2_BIT:
			    rl2_get_pixel_sample_2bit (pxl, &uint8_value);
			    fprintf (stderr, "%u", uint8_value);
			    break;
			case RL2_SAMPLE_4_BIT:
			    rl2_get_pixel_sample_4bit (pxl, &uint8_value);
			    fprintf (stderr, "%u", uint8_value);
			    break;
			case RL2_SAMPLE_INT8:
			    rl2_get_pixel_sample_int8 (pxl, &int8_value);
			    fprintf (stderr, "%d", int8_value);
			    break;
			case RL2_SAMPLE_UINT8:
			    rl2_get_pixel_sample_uint8 (pxl, band_idx,
							&uint8_value);
			    if (band_idx > 0)
				fprintf (stderr, ",%u", uint8_value);
			    else
				fprintf (stderr, "%u", uint8_value);
			    break;
			case RL2_SAMPLE_INT16:
			    rl2_get_pixel_sample_int16 (pxl, &int16_value);
			    fprintf (stderr, "%d", int16_value);
			    break;
			case RL2_SAMPLE_UINT16:
			    rl2_get_pixel_sample_uint16 (pxl, band_idx,
							 &uint16_value);
			    if (band_idx > 0)
				fprintf (stderr, ",%u", uint16_value);
			    else
				fprintf (stderr, "%u", uint16_value);
			    break;
			case RL2_SAMPLE_INT32:
			    rl2_get_pixel_sample_int32 (pxl, &int32_value);
			    fprintf (stderr, "%u", uint32_value);
			    break;
			case RL2_SAMPLE_UINT32:
			    rl2_get_pixel_sample_uint32 (pxl, &uint32_value);
			    fprintf (stderr, "%u", uint32_value);
			    break;
			case RL2_SAMPLE_FLOAT:
			    rl2_get_pixel_sample_float (pxl, &flt_value);
			    dummy = formatFloat (flt_value);
			    fprintf (stderr, "%s", dummy);
			    free (dummy);
			    break;
			case RL2_SAMPLE_DOUBLE:
			    rl2_get_pixel_sample_double (pxl, &dbl_value);
			    dummy = formatFloat (dbl_value);
			    fprintf (stderr, "%s", dummy);
			    free (dummy);
			    break;
			};
		  }
		fprintf (stderr, "\n");
	    }
      }
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
      case RL2_COMPRESSION_CCITTFAX3:
	  fprintf (stderr, "          Compression: CCITT FAX3, lossless\n");
	  *quality = 100;
	  break;
      case RL2_COMPRESSION_CCITTFAX4:
	  fprintf (stderr, "          Compression: CCITT FAX4, lossless\n");
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
    if (!err)
	fprintf (stderr, "        Image Size: %u x %u\n", *width, *height);
    if (dst_path == NULL)
      {
	  fprintf (stderr,
		   "*** ERROR *** no output Destination path was specified\n");
	  err = 1;
      }
    if (*minx == DBL_MAX && *miny == DBL_MAX && *maxx == DBL_MAX
	&& *maxy == DBL_MAX)
      {
	  /* tie-point: Center Point */
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
	  if (err_bbox)
	      goto error;
	  ext_x = (double) (*width) * x_res;
	  ext_y = (double) (*height) * y_res;
	  *minx = *cx - (ext_x / 2.0);
	  *maxx = *minx + ext_x;
	  *miny = *cy - (ext_y / 2.0);
	  *maxy = *miny + ext_y;
      }
    else if (*cx == DBL_MAX && *cy == DBL_MAX && *maxx == DBL_MAX
	     && *maxy == DBL_MAX)
      {
	  /* tie-point: LowerLeft Corner */
	  if (*minx == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Min-X\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*miny == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Min-Y\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (err_bbox)
	      goto error;
	  ext_x = (double) (*width) * x_res;
	  ext_y = (double) (*height) * y_res;
	  *maxx = *minx + ext_x;
	  *maxy = *miny + ext_y;
	  *cx = *minx + (ext_x / 2.0);
	  *cy = *miny + (ext_y / 2.0);
      }
    else if (*cx == DBL_MAX && *cy == DBL_MAX && *minx == DBL_MAX
	     && *maxy == DBL_MAX)
      {
	  /* tie-point: LowerRight Corner */
	  if (*maxx == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Max-X\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*miny == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Min-Y\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (err_bbox)
	      goto error;
	  ext_x = (double) (*width) * x_res;
	  ext_y = (double) (*height) * y_res;
	  *minx = *maxx - ext_x;
	  *maxy = *miny + ext_y;
	  *cx = *maxx - (ext_x / 2.0);
	  *cy = *miny + (ext_y / 2.0);
      }
    else if (*cx == DBL_MAX && *cy == DBL_MAX && *maxx == DBL_MAX
	     && *miny == DBL_MAX)
      {
	  /* tie-point: UpperLeft Corner */
	  if (*minx == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Min-X\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*maxy == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Max-Y\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (err_bbox)
	      goto error;
	  ext_x = (double) (*width) * x_res;
	  ext_y = (double) (*height) * y_res;
	  *maxx = *minx + ext_x;
	  *miny = *maxy - ext_y;
	  *cx = *minx + (ext_x / 2.0);
	  *cy = *maxy - (ext_y / 2.0);
      }
    else if (*cx == DBL_MAX && *cy == DBL_MAX && *minx == DBL_MAX
	     && *miny == DBL_MAX)
      {
	  /* tie-point: UpperRight Corner */
	  if (*maxx == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Max-X\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (*maxy == DBL_MAX)
	    {
		fprintf (stderr, "*** ERROR *** undeclared Max-Y\n");
		err = 1;
		err_bbox = 1;
	    }
	  if (err_bbox)
	      goto error;
	  ext_x = (double) (*width) * x_res;
	  ext_y = (double) (*height) * y_res;
	  *minx = *maxx - ext_x;
	  *miny = *maxy - ext_y;
	  *cx = *maxx - (ext_x / 2.0);
	  *cy = *maxy - (ext_y / 2.0);
      }
    else
      {
	  /* invalid tie-point */
	  fprintf (stderr, "*** ERROR *** invalid output image tie-point\n");
	  err = 1;
	  err_bbox = 1;
      }
  error:
    if (err_bbox)
      {
	  fprintf (stderr,
		   "*** ERROR *** unable to determine the BBOX of output image\n");
	  err = 1;
      }
    else
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
	  dumb1 = formatLong (*cx);
	  dumb2 = formatLat (*cy);
	  fprintf (stderr, "            Center: %s %s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
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
check_de_pyramidize_args (const char *db_path, const char *coverage,
			  const char *section)
{
/* checking/printing DE-PYRAMIDIZE args */
    int err = 0;
    fprintf (stderr, "\n\nrl2_tool; request is DE-PYRAMIDIZE\n");
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

static char *
get_num (const char *start, const char *end)
{
/* extracting a token */
    char *buf;
    int len = end - start;
    if (len <= 0)
	return NULL;
    buf = malloc (len + 1);
    memcpy (buf, start, len);
    *(buf + len) = '\0';
    return buf;
}

static rl2PixelPtr
parse_no_data (const char *in, unsigned char sample_type,
	       unsigned char pixel_type, unsigned char num_bands)
{
/* attempting to parse a NO-DATA pixel value */
    int band = 0;
    char *num = NULL;
    const char *p;
    const char *start;
    const char *end;
    char int8_value;
    unsigned char uint8_value;
    short int16_value;
    unsigned short uint16_value;
    int int32_value;
    unsigned int uint32_value;
    float flt_value;
    double dbl_value;
    rl2PixelPtr pixel = NULL;

    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
	  num_bands = 1;
	  break;
      case RL2_PIXEL_PALETTE:
	  num_bands = 1;
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  num_bands = 1;
	  break;
      case RL2_PIXEL_RGB:
	  num_bands = 3;
	  break;
      case RL2_PIXEL_DATAGRID:
	  num_bands = 1;
	  break;
      };
    pixel = rl2_create_pixel (sample_type, pixel_type, num_bands);
    if (pixel == NULL)
	return NULL;

    start = in;
    p = in;
    while (1)
      {
	  if (*p == ',' || *p == '\0')
	    {
		end = p;
		switch (sample_type)
		  {
		  case RL2_SAMPLE_1_BIT:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      uint8_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_1bit (pixel, int8_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_2_BIT:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      uint8_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_2bit (pixel, int8_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_4_BIT:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      uint8_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_4bit (pixel, int8_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_INT8:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      int8_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_int8 (pixel, int8_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_UINT8:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      uint8_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_uint8 (pixel, band, uint8_value)
			  != RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_INT16:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      int16_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_int16 (pixel, int16_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_UINT16:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      uint16_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_uint16
			  (pixel, band, uint16_value) != RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_INT32:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      int32_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_int32 (pixel, int32_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_UINT32:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      uint32_value = atoi (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_uint32 (pixel, uint32_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_FLOAT:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      flt_value = atof (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_float (pixel, flt_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  case RL2_SAMPLE_DOUBLE:
		      num = get_num (start, end);
		      if (num == NULL)
			  goto error;
		      dbl_value = atof (num);
		      free (num);
		      num = NULL;
		      if (rl2_set_pixel_sample_double (pixel, dbl_value) !=
			  RL2_OK)
			  goto error;
		      break;
		  };
		if (*p == '\0')
		    break;
		start = p + 1;
		band++;
	    }
	  p++;
      }
    return pixel;

  error:
    if (num != NULL)
	free (num);
    if (pixel != NULL)
	rl2_destroy_pixel (pixel);
    return NULL;
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
	  fprintf (stderr,
		   "-nd or --no-data      pixel     NO-DATA pixel value\n\n");
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
		   "NONE DEFLATE LZMA GIF PNG JPEG WEBP LL_WEBP FAX3 FAX4\n\n");
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
	  fprintf (stderr,
		   "In order to export a raster you are expected to specify:\n");
	  fprintf (stderr,
		   "\t- the intended resolution (-res OR -xres AND -yres)\n");
	  fprintf (stderr, "\t- the output image size (-outw AND -outh)\n");
	  fprintf (stderr, "\t- a single tie-point, defined as one of:\n");
	  fprintf (stderr, "\t\t- Output Image Center point: -cx AND -cy\n");
	  fprintf (stderr,
		   "\t\t- Output Image LowerLeft corner: -minx AND -miny\n");
	  fprintf (stderr,
		   "\t\t- Output Image LowerRight corner: -maxx AND -miny\n");
	  fprintf (stderr,
		   "\t\t- Output Image UpperLeft corner: -minx AND -maxy\n");
	  fprintf (stderr,
		   "\t\t- Output Image UpperRight corner: -maxx AND -maxy\n\n");
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
    if (mode == ARG_NONE || mode == ARG_MODE_DE_PYRAMIDIZE)
      {
	  /* MODE = DE-PYRAMIDIZE */
	  fprintf (stderr, "\nmode: DE-PYRAMIDIZE\n");
	  fprintf (stderr, "will delete Pyramid levels\n");
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
    const char *file_ext = ".tif";
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
    const char *no_data_str = NULL;
    rl2PixelPtr no_data = NULL;
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
	  if (strcasecmp (argv[1], "DE-PYRAMIDIZE") == 0)
	      mode = ARG_MODE_DE_PYRAMIDIZE;
	  if (strcasecmp (argv[1], "LIST") == 0)
	      mode = ARG_MODE_LIST;
	  if (strcasecmp (argv[1], "MAP") == 0)
	      mode = ARG_MODE_MAP;
	  if (strcasecmp (argv[1], "CATALOG") == 0)
	      mode = ARG_MODE_CATALOG;
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
		      if (strcasecmp (argv[i], "WEBP") == 0)
			  compression = RL2_COMPRESSION_LOSSY_WEBP;
		      if (strcasecmp (argv[i], "LL_WEBP") == 0)
			  compression = RL2_COMPRESSION_LOSSLESS_WEBP;
		      if (strcasecmp (argv[i], "FAX3") == 0)
			  compression = RL2_COMPRESSION_CCITTFAX3;
		      if (strcasecmp (argv[i], "FAX4") == 0)
			  compression = RL2_COMPRESSION_CCITTFAX4;
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
		  case ARG_NO_DATA:
		      no_data_str = argv[i];
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
	  if (strcmp (argv[i], "-nd") == 0
	      || strcasecmp (argv[i], "--no-data") == 0)
	    {
		next_arg = ARG_NO_DATA;
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
	  if (no_data_str != NULL)
	    {
		no_data = parse_no_data (no_data_str, sample, pixel, num_bands);
		if (no_data == NULL)
		  {
		      fprintf (stderr,
			       "Unable to parse the NO-DATA pixel !!!\n");
		      error = 1;
		      break;
		  }
	    }
	  switch (pixel)
	    {
	    case RL2_PIXEL_MONOCHROME:
	    case RL2_PIXEL_PALETTE:
	    case RL2_PIXEL_GRAYSCALE:
	    case RL2_PIXEL_DATAGRID:
		if (num_bands == RL2_BANDS_UNKNOWN)
		    num_bands = 1;
		break;
	    case RL2_PIXEL_RGB:
		if (num_bands == RL2_BANDS_UNKNOWN)
		    num_bands = 3;
		break;
	    };
	  error =
	      check_create_args (db_path, coverage, sample, pixel, num_bands,
				 compression, &quality, tile_width, tile_height,
				 srid, x_res, y_res, no_data);
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
      case ARG_MODE_DE_PYRAMIDIZE:
	  error = check_de_pyramidize_args (db_path, coverage, section);
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
			   tile_height, srid, x_res, y_res, no_data);
	  break;
      case ARG_MODE_DROP:
	  ret = exec_drop (handle, coverage);
	  break;
      case ARG_MODE_IMPORT:
	  ret =
	      exec_import (handle, src_path, dir_path, file_ext, coverage,
			   worldfile, srid, pyramidize);
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
      case ARG_MODE_DE_PYRAMIDIZE:
	  ret = exec_de_pyramidize (handle, coverage, section);
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
	    case ARG_MODE_DE_PYRAMIDIZE:
		op_name = "DE-PYRAMIDIZE";
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
