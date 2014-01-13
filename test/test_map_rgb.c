/*

 test_map_rgb.c -- RasterLite-2 Test Case

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

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"

#define TILE_256	256
#define TILE_512	512
#define TILE_1024	1024

static int
test_coverage (sqlite3 * sqlite, unsigned char pixel, unsigned char compression,
	       int tile_sz, int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage;
    const char *sample_name;
    const char *pixel_name;
    unsigned char num_bands;
    const char *compression_name;
    int qlty;
    char *sql;
    int tile_size;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_RGB:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_none_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_none_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_zip_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_zip_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_zip_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_lzma_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_lzma_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_png_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_png_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_JPEG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_jpeg_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_jpeg_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_jpeg_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_webp_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_webp_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_webp_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_llwebp_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_llwebp_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_llwebp_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_none_256";
		      break;
		  case TILE_512:
		      coverage = "gray_none_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_zip_256";
		      break;
		  case TILE_512:
		      coverage = "gray_zip_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_zip_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "gray_lzma_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_lzma_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_png_256";
		      break;
		  case TILE_512:
		      coverage = "gray_png_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_JPEG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_jpeg_256";
		      break;
		  case TILE_512:
		      coverage = "gray_jpeg_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_jpeg_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_webp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_webp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_webp_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_llwebp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_llwebp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_llwebp_1024";
		      break;
		  };
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    sample_name = "UINT8";
    switch (pixel)
      {
      case RL2_PIXEL_RGB:
	  num_bands = 3;
	  pixel_name = "RGB";
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  num_bands = 1;
	  pixel_name = "GRAYSCALE";
	  break;
      };
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
      case RL2_COMPRESSION_LZMA:
	  compression_name = "LZMA";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_PNG:
	  compression_name = "PNG";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_JPEG:
	  compression_name = "JPEG";
	  qlty = 80;
	  break;
      case RL2_COMPRESSION_LOSSY_WEBP:
	  compression_name = "LOSSY_WEBP";
	  qlty = 80;
	  break;
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  compression_name = "LOSSLESS_WEBP";
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
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.16f, %1.16f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 26914,
			   0.152400030480006134, 0.152400030480006134);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
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
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 26914, 1)", coverage,
	 "map_samples/usgs-rgb", ".tif");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
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
    sql = sqlite3_mprintf ("SELECT RL2_DeleteSection(%Q, %Q, 1)",
			   coverage, "rgb1");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
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
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, 26914, 1)",
			   coverage, "map_samples/usgs-rgb/rgb1.tif");
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
	  return 0;
      }


    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char pixel, unsigned char compression,
	       int tile_sz, int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage;
    char *sql;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_RGB:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_none_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_none_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_zip_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_zip_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_zip_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_lzma_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_lzma_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_png_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_png_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_JPEG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_jpeg_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_jpeg_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_jpeg_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_webp_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_webp_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_webp_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "rgb_llwebp_256";
		      break;
		  case TILE_512:
		      coverage = "rgb_llwebp_512";
		      break;
		  case TILE_1024:
		      coverage = "rgb_llwebp_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_none_256";
		      break;
		  case TILE_512:
		      coverage = "gray_none_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_zip_256";
		      break;
		  case TILE_512:
		      coverage = "gray_zip_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_zip_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_lzma_256";
		      break;
		  case TILE_512:
		      coverage = "gray_lzma_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_lzma_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_png_256";
		      break;
		  case TILE_512:
		      coverage = "gray_png_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_JPEG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_jpeg_256";
		      break;
		  case TILE_512:
		      coverage = "gray_jpeg_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_jpeg_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_webp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_webp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_webp_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_llwebp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_llwebp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_llwebp_1024";
		      break;
		  };
		break;
	    };
	  break;
      };

/* dropping the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_DropCoverage(%Q, 1)", coverage);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
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

/* RGB tests */
    ret = -100;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -120;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -140;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -200;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -220;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -240;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -300;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, TILE_256, &ret))
	return ret;
    ret = -320;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, TILE_512, &ret))
	return ret;
    ret = -340;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, TILE_1024, &ret))
	return ret;
    ret = -400;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP, TILE_256, &ret))
	return ret;
    ret = -420;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP, TILE_512, &ret))
	return ret;
    ret = -440;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP, TILE_1024, &ret))
	return ret;
    ret = -500;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP, TILE_256,
	 &ret))
	return ret;
    ret = -520;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP, TILE_512,
	 &ret))
	return ret;
    ret = -540;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP, TILE_1024,
	 &ret))
	return ret;

/* GRAYSCALE tests */
    ret = -600;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -620;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -640;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -700;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -720;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -740;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -800;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_256, &ret))
	return ret;
    ret = -820;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_512, &ret))
	return ret;
    ret = -840;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_1024, &ret))
	return ret;
    ret = -900;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_256,
	 &ret))
	return ret;
    ret = -920;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_512,
	 &ret))
	return ret;
    ret = -940;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_1024,
	 &ret))
	return ret;
    ret = -1000;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_256, &ret))
	return ret;
    ret = -1020;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_512, &ret))
	return ret;
    ret = -1040;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_1024, &ret))
	return ret;

/* dropping all RGB Coverages */
    ret = -170;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -190;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -270;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -290;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -370;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, TILE_256, &ret))
	return ret;
    ret = -380;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, TILE_512, &ret))
	return ret;
    ret = -390;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, TILE_1024, &ret))
	return ret;
    ret = -470;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP, TILE_256, &ret))
	return ret;
    ret = -480;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP, TILE_512, &ret))
	return ret;
    ret = -490;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP, TILE_1024, &ret))
	return ret;
    ret = -570;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP, TILE_256,
	 &ret))
	return ret;
    ret = -580;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP, TILE_512,
	 &ret))
	return ret;
    ret = -590;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP, TILE_1024,
	 &ret))
	return ret;

/* dropping all GRAYSCALE Coverages */
    ret = -170;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -190;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -270;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -290;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -370;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_256, &ret))
	return ret;
    ret = -380;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_512, &ret))
	return ret;
    ret = -390;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_1024, &ret))
	return ret;
    ret = -470;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_256,
	 &ret))
	return ret;
    ret = -480;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_512,
	 &ret))
	return ret;
    ret = -490;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_1024,
	 &ret))
	return ret;
    ret = -570;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_256, &ret))
	return ret;
    ret = -580;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_512, &ret))
	return ret;
    ret = -590;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_1024, &ret))
	return ret;

/* closing the DB */
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
