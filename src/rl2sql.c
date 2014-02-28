/*

 rl2sql -- main SQLite extension methods

 version 0.1, 2013 December 22

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

static void
fnct_rl2_version (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_version()
/
/ return a text string representing the current RasterLite-2 version
*/
    int len;
    const char *p_result = rl2_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_target_cpu (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_target_cpu()
/
/ return a text string representing the current RasterLite-2 Target CPU
*/
    int len;
    const char *p_result = rl2_target_cpu ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_IsValidPixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsValidPixel(BLOBencoded pixel, text sample_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *sample;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	sqlite3_result_int (context, -1);
    else
      {
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  bands = sqlite3_value_int (argv[2]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
	  ret = rl2_is_valid_dbms_pixel (blob, blob_sz, sample_type, num_bands);
	  if (ret == RL2_OK)
	      sqlite3_result_int (context, 1);
	  else
	      sqlite3_result_int (context, 0);
      }
}

static void
fnct_IsValidRasterPalette (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterPalette(BLOBencoded palette, text sample_type, text pixel_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *sample;
    const char *pixel;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	sqlite3_result_int (context, -1);
    else
      {
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  pixel = (const char *) sqlite3_value_text (argv[2]);
	  bands = sqlite3_value_int (argv[3]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (strcmp (pixel, "MONOCHROME") == 0)
	      pixel_type = RL2_PIXEL_MONOCHROME;
	  if (strcmp (pixel, "GRAYSCALE") == 0)
	      pixel_type = RL2_PIXEL_GRAYSCALE;
	  if (strcmp (pixel, "PALETTE") == 0)
	      pixel_type = RL2_PIXEL_PALETTE;
	  if (strcmp (pixel, "RGB") == 0)
	      pixel_type = RL2_PIXEL_RGB;
	  if (strcmp (pixel, "MULTIBAND") == 0)
	      pixel_type = RL2_PIXEL_MULTIBAND;
	  if (strcmp (pixel, "DATAGRID") == 0)
	      pixel_type = RL2_PIXEL_DATAGRID;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || pixel_type == RL2_PIXEL_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
	  ret =
	      rl2_is_valid_dbms_palette (blob, blob_sz, sample_type, pixel_type,
					 num_bands);
	  if (ret == RL2_OK)
	      sqlite3_result_int (context, 1);
	  else
	      sqlite3_result_int (context, 0);
      }
}

static void
fnct_IsValidRasterStatistics (sqlite3_context * context, int argc,
			      sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterStatistics(text covarage, BLOBencoded statistics)
/   or
/ IsValidRasterStatistics((BLOBencoded statistics, text sample_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *coverage;
    const char *sample;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    sqlite3 *sqlite;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (argc == 3)
      {
	  if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (err)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  bands = sqlite3_value_int (argv[2]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
      }
    else
      {
	  if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	      err = 1;
	  if (err)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
	  sqlite = sqlite3_context_db_handle (context);
	  coverage = (const char *) sqlite3_value_text (argv[0]);
	  blob = sqlite3_value_blob (argv[1]);
	  blob_sz = sqlite3_value_bytes (argv[1]);
	  if (!get_coverage_sample_bands
	      (sqlite, coverage, &sample_type, &num_bands))
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret =
	rl2_is_valid_dbms_raster_statistics (blob, blob_sz, sample_type,
					     num_bands);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_IsValidRasterTile (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterTile(text coverage, integer level, BLOBencoded tile_odd,
/   BLOBencoded tile_even)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    int level;
    const unsigned char *blob_odd;
    int blob_odd_sz;
    const unsigned char *blob_even;
    int blob_even_sz;
    const char *coverage;
    unsigned short tile_width;
    unsigned short tile_height;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    unsigned char compression = RL2_COMPRESSION_UNKNOWN;
    sqlite3 *sqlite;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_BLOB
	&& sqlite3_value_type (argv[3]) != SQLITE_NULL)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    level = sqlite3_value_int (argv[1]);
    blob_odd = sqlite3_value_blob (argv[2]);
    blob_odd_sz = sqlite3_value_bytes (argv[2]);
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
      {
	  blob_even = NULL;
	  blob_even_sz = 0;
      }
    else
      {
	  blob_even = sqlite3_value_blob (argv[3]);
	  blob_even_sz = sqlite3_value_bytes (argv[3]);
      }
    if (!get_coverage_defs
	(sqlite, coverage, &tile_width, &tile_height, &sample_type, &pixel_type,
	 &num_bands, &compression))
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    ret =
	rl2_is_valid_dbms_raster_tile (level, tile_width, tile_height, blob_odd,
				       blob_odd_sz, blob_even, blob_even_sz,
				       sample_type, pixel_type, num_bands,
				       compression);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_CreatePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ CreatePixel(text sample_type, text pixel_type, int num_bands)
/
/ will return a serialized binary Pixel Object 
/ or NULL on failure
*/
    int err = 0;
    const char *sample_type;
    const char *pixel_type;
    int num_bands;
    unsigned char sample;
    unsigned char pixel;
    rl2PixelPtr pxl = NULL;
    unsigned char *blob = NULL;
    int blob_sz = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto error;

/* retrieving the arguments */
    sample_type = (const char *) sqlite3_value_text (argv[0]);
    pixel_type = (const char *) sqlite3_value_text (argv[1]);
    num_bands = sqlite3_value_int (argv[2]);

/* preliminary arg checking */
    if (num_bands < 1 || num_bands > 255)
	goto error;

    sample = RL2_SAMPLE_UNKNOWN;
    if (strcasecmp (sample_type, "1-BIT") == 0)
	sample = RL2_SAMPLE_1_BIT;
    if (strcasecmp (sample_type, "2-BIT") == 0)
	sample = RL2_SAMPLE_2_BIT;
    if (strcasecmp (sample_type, "4-BIT") == 0)
	sample = RL2_SAMPLE_4_BIT;
    if (strcasecmp (sample_type, "INT8") == 0)
	sample = RL2_SAMPLE_INT8;
    if (strcasecmp (sample_type, "UINT8") == 0)
	sample = RL2_SAMPLE_UINT8;
    if (strcasecmp (sample_type, "INT16") == 0)
	sample = RL2_SAMPLE_INT16;
    if (strcasecmp (sample_type, "UINT16") == 0)
	sample = RL2_SAMPLE_UINT16;
    if (strcasecmp (sample_type, "INT32") == 0)
	sample = RL2_SAMPLE_INT32;
    if (strcasecmp (sample_type, "UINT32") == 0)
	sample = RL2_SAMPLE_UINT32;
    if (strcasecmp (sample_type, "FLOAT") == 0)
	sample = RL2_SAMPLE_FLOAT;
    if (strcasecmp (sample_type, "DOUBLE") == 0)
	sample = RL2_SAMPLE_DOUBLE;

    pixel = RL2_PIXEL_UNKNOWN;
    if (strcasecmp (pixel_type, "MONOCHROME") == 0)
	pixel = RL2_PIXEL_MONOCHROME;
    if (strcasecmp (pixel_type, "GRAYSCALE") == 0)
	pixel = RL2_PIXEL_GRAYSCALE;
    if (strcasecmp (pixel_type, "PALETTE") == 0)
	pixel = RL2_PIXEL_PALETTE;
    if (strcasecmp (pixel_type, "RGB") == 0)
	pixel = RL2_PIXEL_RGB;
    if (strcasecmp (pixel_type, "DATAGRID") == 0)
	pixel = RL2_PIXEL_DATAGRID;
    if (strcasecmp (pixel_type, "MULTIBAND") == 0)
	pixel = RL2_PIXEL_MULTIBAND;

/* attempting to create the Pixel */
    pxl = rl2_create_pixel (sample, pixel, (unsigned char) num_bands);
    if (pxl == NULL)
	goto error;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelType (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelType(BLOB pixel_obj)
/
/ will return one of "1-BIT", "2-BIT", "4-BIT", "INT8", "UINT8", "INT16",
/      "UINT16", "INT32", "UINT32", "FLOAT", "DOUBLE", "UNKNOWN" 
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    const char *text;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
	  text = "1-BIT";
	  break;
      case RL2_SAMPLE_2_BIT:
	  text = "2-BIT";
	  break;
      case RL2_SAMPLE_4_BIT:
	  text = "4-BIT";
	  break;
      case RL2_SAMPLE_INT8:
	  text = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  text = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  text = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  text = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  text = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  text = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  text = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  text = "DOUBLE";
	  break;
      default:
	  text = "UNKNOWN";
	  break;
      };
    sqlite3_result_text (context, text, strlen (text), SQLITE_TRANSIENT);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelSampleType (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelSampleType(BLOB pixel_obj)
/
/ will return one of "MONOCHROME" "PALETTE", "GRAYSCALE", "RGB",
/      "DATAGRID", "MULTIBAND", "UNKNOWN" 
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    const char *text;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    switch (pixel->pixelType)
      {
      case RL2_PIXEL_MONOCHROME:
	  text = "MONOCHROME";
	  break;
      case RL2_PIXEL_PALETTE:
	  text = "PALETTE";
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  text = "GRAYSCALE";
	  break;
      case RL2_PIXEL_RGB:
	  text = "RGB";
	  break;
      case RL2_PIXEL_DATAGRID:
	  text = "DATAGRID";
	  break;
      case RL2_PIXEL_MULTIBAND:
	  text = "MULTIBAND";
	  break;
      default:
	  text = "UNKNOWN";
	  break;
      };
    sqlite3_result_text (context, text, strlen (text), SQLITE_TRANSIENT);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelNumBands (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelNumBands(BLOB pixel_obj)
/
/ will return the number of Bands
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    sqlite3_result_int (context, pixel->nBands);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelValue (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelValue(BLOB pixel_obj, INT band)
/
/ will return the corresponding Band value
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    rl2PrivSamplePtr sample;
    int band_id;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_id = sqlite3_value_int (argv[1]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (band_id >= 0 && band_id < pixel->nBands)
	;
    else
	goto error;
    sample = pixel->Samples + band_id;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  sqlite3_result_int (context, sample->int8);
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  sqlite3_result_int (context, sample->uint8);
	  break;
      case RL2_SAMPLE_INT16:
	  sqlite3_result_int (context, sample->int16);
	  break;
      case RL2_SAMPLE_UINT16:
	  sqlite3_result_int (context, sample->uint16);
	  break;
      case RL2_SAMPLE_INT32:
	  sqlite3_result_int64 (context, sample->int32);
	  break;
      case RL2_SAMPLE_UINT32:
	  sqlite3_result_int64 (context, sample->uint32);
	  break;
      case RL2_SAMPLE_FLOAT:
	  sqlite3_result_double (context, sample->float32);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sqlite3_result_double (context, sample->float64);
	  break;
      default:
	  goto error;
      };
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetPixelValue (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ SetPixelValue(BLOB pixel_obj, INT band, INT value)
/ SetPixelValue(BLOB pixel_obj, INT band, FLOAT value)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    rl2PrivSamplePtr sample;
    int band_id;
    int intval;
    double dblval;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[2]) != SQLITE_FLOAT)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_id = sqlite3_value_int (argv[1]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (band_id >= 0 && band_id < pixel->nBands)
	;
    else
	goto error;
    if (pixel->sampleType == RL2_SAMPLE_FLOAT
	|| pixel->sampleType == RL2_SAMPLE_DOUBLE)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      dblval = sqlite3_value_double (argv[2]);
	  else
	    {
		intval = sqlite3_value_int (argv[2]);
		dblval = intval;
	    }
      }
    else
      {
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      goto error;
	  intval = sqlite3_value_int (argv[2]);
      }
    sample = pixel->Samples + band_id;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  if (intval < INT8_MIN || intval > INT8_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_1_BIT:
	  if (intval < 0 || intval > 1)
	      goto error;
	  break;
      case RL2_SAMPLE_2_BIT:
	  if (intval < 0 || intval > 3)
	      goto error;
	  break;
      case RL2_SAMPLE_4_BIT:
	  if (intval < 0 || intval > 15)
	      goto error;
	  break;
      case RL2_SAMPLE_UINT8:
	  if (intval < 0 || intval > UINT8_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_INT16:
	  if (intval < INT16_MIN || intval > INT16_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_UINT16:
	  if (intval < 0 || intval > UINT16_MAX)
	      goto error;
	  break;
      };
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  sample->int8 = intval;
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  sample->uint8 = intval;
	  break;
      case RL2_SAMPLE_INT16:
	  sample->int16 = intval;
	  break;
      case RL2_SAMPLE_UINT16:
	  sample->uint16 = intval;
	  break;
      case RL2_SAMPLE_INT32:
	  sample->int32 = intval;
	  break;
      case RL2_SAMPLE_UINT32:
	  sample->uint32 = (unsigned int) intval;
	  break;
      case RL2_SAMPLE_FLOAT:
	  sample->float32 = dblval;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample->float64 = dblval;
	  break;
      default:
	  goto error;
      };
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_IsTransparentPixel (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ IsTransparentPixel(BLOB pixel_obj)
/
/ 1 (TRUE) or 0 (FALSE)
/ or NULL on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (pixel->isTransparent == 0)
	sqlite3_result_int (context, 0);
    else
	sqlite3_result_int (context, 1);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_IsOpaquePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsOpaquePixel(BLOB pixel_obj)
/
/ 1 (TRUE) or 0 (FALSE)
/ or NULL on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (pixel->isTransparent == 0)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetTransparentPixel (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ SetTransparentPixel(BLOB pixel_obj)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    pixel->isTransparent = 1;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetOpaquePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ SetOpaquePixel(BLOB pixel_obj)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    pixel->isTransparent = 0;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_PixelEquals (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ PixelEquals(BLOB pixel_obj1, BLOB pixel_obj2)
/
/ 1 (TRUE) or 0 (FALSE)
/ or NULL on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl1 = NULL;
    rl2PixelPtr pxl2 = NULL;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl1 = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl1 == NULL)
	goto error;
    blob = sqlite3_value_blob (argv[1]);
    blob_sz = sqlite3_value_bytes (argv[1]);
    pxl2 = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl2 == NULL)
	goto error;
    ret = rl2_compare_pixels (pxl1, pxl2);
    if (ret == RL2_TRUE)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    rl2_destroy_pixel (pxl1);
    rl2_destroy_pixel (pxl2);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl1 != NULL)
	rl2_destroy_pixel (pxl1);
    if (pxl2 != NULL)
	rl2_destroy_pixel (pxl2);
}

static void
fnct_CreateCoverage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double res)
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double horz_res, double vert_res)
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double horz_res, double vert_res, BLOB no_data)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    const char *sample_type;
    const char *pixel_type;
    int num_bands;
    const char *compression;
    int quality;
    int tile_width;
    int tile_height;
    int srid;
    double horz_res;
    double vert_res;
    unsigned char sample;
    unsigned char pixel;
    unsigned char compr;
    sqlite3 *sqlite;
    int ret;
    rl2PixelPtr no_data = NULL;
    rl2PalettePtr palette = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 10)
      {
	  if (sqlite3_value_type (argv[10]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[10]) != SQLITE_FLOAT)
	      err = 1;
      }
    if (argc > 11)
      {
	  if (sqlite3_value_type (argv[11]) != SQLITE_BLOB
	      && sqlite3_value_type (argv[11]) != SQLITE_NULL)
	      err = 1;
      }
    if (err)
	goto error;

/* retrieving the arguments */
    coverage = (const char *) sqlite3_value_text (argv[0]);
    sample_type = (const char *) sqlite3_value_text (argv[1]);
    pixel_type = (const char *) sqlite3_value_text (argv[2]);
    num_bands = sqlite3_value_int (argv[3]);
    compression = (const char *) sqlite3_value_text (argv[4]);
    quality = sqlite3_value_int (argv[5]);
    tile_width = sqlite3_value_int (argv[6]);
    tile_height = sqlite3_value_int (argv[7]);
    srid = sqlite3_value_int (argv[8]);
    if (sqlite3_value_type (argv[9]) == SQLITE_FLOAT)
	horz_res = sqlite3_value_double (argv[9]);
    else
      {
	  int val = sqlite3_value_int (argv[9]);
	  horz_res = val;
      }
    if (argc > 10)
      {
	  if (sqlite3_value_type (argv[10]) == SQLITE_FLOAT)
	      vert_res = sqlite3_value_double (argv[10]);
	  else
	    {
		int val = sqlite3_value_int (argv[10]);
		vert_res = val;
	    }
      }
    else
	vert_res = horz_res;
    if (argc > 11 && sqlite3_value_type (argv[11]) == SQLITE_BLOB)
      {
	  /* NO-DATA pixel */
	  const unsigned char *blob = sqlite3_value_blob (argv[11]);
	  int blob_sz = sqlite3_value_bytes (argv[11]);
	  no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
	  if (no_data == NULL)
	      goto error;
      }

/* preliminary arg checking */
    if (num_bands < 1 || num_bands > 255)
	goto error;
    if (quality < 0)
	quality = 0;
    if (quality > 100)
	quality = 100;
    if (tile_width < 0 || tile_width > 65536)
	goto error;
    if (tile_height < 0 || tile_height > 65536)
	goto error;

    sample = RL2_SAMPLE_UNKNOWN;
    if (strcasecmp (sample_type, "1-BIT") == 0)
	sample = RL2_SAMPLE_1_BIT;
    if (strcasecmp (sample_type, "2-BIT") == 0)
	sample = RL2_SAMPLE_2_BIT;
    if (strcasecmp (sample_type, "4-BIT") == 0)
	sample = RL2_SAMPLE_4_BIT;
    if (strcasecmp (sample_type, "INT8") == 0)
	sample = RL2_SAMPLE_INT8;
    if (strcasecmp (sample_type, "UINT8") == 0)
	sample = RL2_SAMPLE_UINT8;
    if (strcasecmp (sample_type, "INT16") == 0)
	sample = RL2_SAMPLE_INT16;
    if (strcasecmp (sample_type, "UINT16") == 0)
	sample = RL2_SAMPLE_UINT16;
    if (strcasecmp (sample_type, "INT32") == 0)
	sample = RL2_SAMPLE_INT32;
    if (strcasecmp (sample_type, "UINT32") == 0)
	sample = RL2_SAMPLE_UINT32;
    if (strcasecmp (sample_type, "FLOAT") == 0)
	sample = RL2_SAMPLE_FLOAT;
    if (strcasecmp (sample_type, "DOUBLE") == 0)
	sample = RL2_SAMPLE_DOUBLE;

    pixel = RL2_PIXEL_UNKNOWN;
    if (strcasecmp (pixel_type, "MONOCHROME") == 0)
	pixel = RL2_PIXEL_MONOCHROME;
    if (strcasecmp (pixel_type, "GRAYSCALE") == 0)
	pixel = RL2_PIXEL_GRAYSCALE;
    if (strcasecmp (pixel_type, "PALETTE") == 0)
	pixel = RL2_PIXEL_PALETTE;
    if (strcasecmp (pixel_type, "RGB") == 0)
	pixel = RL2_PIXEL_RGB;
    if (strcasecmp (pixel_type, "DATAGRID") == 0)
	pixel = RL2_PIXEL_DATAGRID;
    if (strcasecmp (pixel_type, "MULTIBAND") == 0)
	pixel = RL2_PIXEL_MULTIBAND;

    compr = RL2_COMPRESSION_UNKNOWN;
    if (strcasecmp (compression, "NONE") == 0)
	compr = RL2_COMPRESSION_NONE;
    if (strcasecmp (compression, "DEFLATE") == 0)
	compr = RL2_COMPRESSION_DEFLATE;
    if (strcasecmp (compression, "LZMA") == 0)
	compr = RL2_COMPRESSION_LZMA;
    if (strcasecmp (compression, "PNG") == 0)
	compr = RL2_COMPRESSION_PNG;
    if (strcasecmp (compression, "GIF") == 0)
	compr = RL2_COMPRESSION_GIF;
    if (strcasecmp (compression, "JPEG") == 0)
	compr = RL2_COMPRESSION_JPEG;
    if (strcasecmp (compression, "WEBP") == 0)
	compr = RL2_COMPRESSION_LOSSY_WEBP;
    if (strcasecmp (compression, "LL_WEBP") == 0)
	compr = RL2_COMPRESSION_LOSSLESS_WEBP;
    if (strcasecmp (compression, "FAX4") == 0)
	compr = RL2_COMPRESSION_CCITTFAX4;

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

/* attempting to create the DBMS Coverage */
    sqlite = sqlite3_context_db_handle (context);
    ret = rl2_create_dbms_coverage (sqlite, coverage, sample, pixel,
				    (unsigned char) num_bands,
				    compr, quality,
				    (unsigned short) tile_width,
				    (unsigned short) tile_height, srid,
				    horz_res, vert_res, no_data, palette);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return;

  error:
    sqlite3_result_int (context, -1);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
}

static void
fnct_DeleteSection (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ DeleteSection(text coverage, text section)
/ DeleteSection(text coverage, text section, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    const char *section;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    rl2CoveragePtr cvg = NULL;
    sqlite3_int64 section_id;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto invalid;

/* retrieving the arguments */
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    section = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	transaction = sqlite3_value_int (argv[2]);

    cvg = rl2_create_coverage_from_dbms (sqlite, coverage);
    if (cvg == NULL)
	goto error;
    if (rl2_get_dbms_section_id (sqlite, coverage, section, &section_id) !=
	RL2_OK)
	goto error;

    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    if (rl2_delete_dbms_section (sqlite, coverage, section_id) != RL2_OK)
	goto error;

    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }
    sqlite3_result_int (context, 1);
    rl2_destroy_coverage (cvg);
    return;

  invalid:
    sqlite3_result_int (context, -1);
    return;
  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    sqlite3_result_int (context, 0);
    if (transaction)
      {
	  /* invalidating the pending transaction */
	  sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
      }
    return;
}

static void
fnct_DropCoverage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ DropCoverage(text coverage)
/ DropCoverage(text coverage, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    rl2CoveragePtr cvg = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto invalid;

/* retrieving the arguments */
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    if (argc > 1)
	transaction = sqlite3_value_int (argv[1]);

    cvg = rl2_create_coverage_from_dbms (sqlite, coverage);
    if (cvg == NULL)
	goto error;

    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    if (rl2_drop_dbms_coverage (sqlite, coverage) != RL2_OK)
	goto error;

    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }
    sqlite3_result_int (context, 1);
    rl2_destroy_coverage (cvg);
    return;

  invalid:
    sqlite3_result_int (context, -1);
    return;
  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    sqlite3_result_int (context, 0);
    if (transaction)
      {
	  /* invalidating the pending transaction */
	  sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
      }
    return;
}

static void
fnct_LoadRaster (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ LoadRaster(text coverage, text path_to_raster)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid, int pyramidize)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid, int pyramidize, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int worldfile = 0;
    int force_srid = -1;
    int transaction = 1;
    int pyramidize = 1;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	worldfile = sqlite3_value_int (argv[2]);
    if (argc > 3)
	force_srid = sqlite3_value_int (argv[3]);
    if (argc > 4)
	pyramidize = sqlite3_value_int (argv[4]);
    if (argc > 5)
	transaction = sqlite3_value_int (argv[5]);


/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* attempting to load the Raster into the DBMS */
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret = rl2_load_raster_into_dbms (sqlite, path, coverage,
				     worldfile, force_srid, pyramidize);
    rl2_destroy_coverage (coverage);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_LoadRastersFromDir (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ LoadRastersFromDir(text coverage, text dir_path)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid, 
/                    int pyramidize)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid, 
/                    int pyramidize, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    const char *file_ext;
    int worldfile = 0;
    int force_srid = -1;
    int transaction = 1;
    int pyramidize = 1;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	file_ext = (const char *) sqlite3_value_text (argv[2]);
    if (argc > 3)
	worldfile = sqlite3_value_int (argv[3]);
    if (argc > 4)
	force_srid = sqlite3_value_int (argv[4]);
    if (argc > 5)
	pyramidize = sqlite3_value_int (argv[5]);
    if (argc > 6)
	transaction = sqlite3_value_int (argv[6]);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* attempting to load the Rasters into the DBMS */
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret = rl2_load_mrasters_into_dbms (sqlite, path, file_ext, coverage,
				       worldfile, force_srid, pyramidize);
    rl2_destroy_coverage (coverage);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_Pyramidize (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ Pyramidize(text coverage)
/ Pyramidize(text coverage, text section)
/ Pyramidize(text coverage, text section, int force_rebuild)
/ Pyramidize(text coverage, text section, int force_rebuild,
/            int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *sect_name = NULL;
    int forced_rebuild = 0;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_TEXT
	&& sqlite3_value_type (argv[1]) != SQLITE_NULL)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
/* attempting to (re)build Pyramid levels */
    sqlite = sqlite3_context_db_handle (context);
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    if (argc > 1)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	      sect_name = (const char *) sqlite3_value_text (argv[1]);
      }
    if (argc > 2)
	forced_rebuild = sqlite3_value_int (argv[2]);
    if (argc > 3)
	transaction = sqlite3_value_int (argv[3]);
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (sect_name == NULL)
	ret = rl2_build_all_section_pyramids (sqlite, cvg_name, forced_rebuild);
    else
	ret =
	    rl2_build_section_pyramid (sqlite, cvg_name, sect_name,
				       forced_rebuild);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_DePyramidize (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ DePyramidize(text coverage)
/ DePyramidize(text coverage, text section)
/ DePyramidize(text coverage, text section, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *sect_name = NULL;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_TEXT
	&& sqlite3_value_type (argv[1]) != SQLITE_NULL)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
/* attempting to delete all Pyramid levels */
    sqlite = sqlite3_context_db_handle (context);
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    if (argc > 1)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	      sect_name = (const char *) sqlite3_value_text (argv[1]);
      }
    if (argc > 2)
	transaction = sqlite3_value_int (argv[2]);
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (sect_name == NULL)
	ret = rl2_delete_all_pyramids (sqlite, cvg_name);
    else
	ret = rl2_delete_section_pyramid (sqlite, cvg_name, sect_name);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_LoadRasterFromWMS (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double res)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer,
/                   text wms_style, text wms_format, double horz_res, 
/                   double vert_res)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, text wms_crs,
/                   double horz_res, double vert_res, int opaque)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double horz_res,
/                   double vert_res, int opaque,
/                   int swap_xy)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double horz_res,
/                   double vert_res, int opaque, int swap_xy, text proxy)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double horz_res,
/                   double vert_res,  int opaque, int swap_xy, 
/                   text proxy, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *sect_name;
    const char *url;
    const char *proxy = NULL;
    const char *wms_version;
    const char *wms_layer;
    const char *wms_style = NULL;
    const char *wms_format;
    char *wms_crs = NULL;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int opaque = 0;
    int swap_xy = 0;
    int srid;
    int transaction = 1;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    rl2CoveragePtr coverage = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    sqlite3 *sqlite;
    int ret;
    int n;
    double x;
    double y;
    double tilew;
    double tileh;
    unsigned short tile_width;
    unsigned short tile_height;
    WmsRetryListPtr retry_list = NULL;
    char *table;
    char *xtable;
    char *sql;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_upd_sect = NULL;
    int first = 1;
    double ext_x;
    double ext_y;
    int width;
    int height;
    sqlite3_int64 section_id;
    rl2PixelPtr no_data = NULL;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int quality;
    InsertWms params;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_TEXT
	&& sqlite3_value_type (argv[6]) != SQLITE_NULL)
	err = 1;
    if (sqlite3_value_type (argv[7]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 12)
      {
	  if (sqlite3_value_type (argv[12]) == SQLITE_TEXT)
	      ;
	  else if (sqlite3_value_type (argv[12]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
      }
    if (argc > 13 && sqlite3_value_type (argv[13]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    sect_name = (const char *) sqlite3_value_text (argv[1]);
    url = (const char *) sqlite3_value_text (argv[2]);
    blob = sqlite3_value_blob (argv[3]);
    blob_sz = sqlite3_value_bytes (argv[3]);
    wms_version = (const char *) sqlite3_value_text (argv[4]);
    wms_layer = (const char *) sqlite3_value_text (argv[5]);
    if (sqlite3_value_type (argv[6]) == SQLITE_TEXT)
	wms_style = (const char *) sqlite3_value_text (argv[6]);
    wms_format = (const char *) sqlite3_value_text (argv[7]);
    if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[8]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[8]);
    if (argc > 9)
      {
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[9]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[9]);
      }
    else
	vert_res = horz_res;
    if (argc > 10)
	opaque = sqlite3_value_int (argv[10]);
    if (argc > 11)
	swap_xy = sqlite3_value_int (argv[11]);
    if (argc > 12 && sqlite3_value_type (argv[12]) == SQLITE_TEXT)
	proxy = (const char *) sqlite3_value_text (argv[12]);
    if (argc > 13)
	transaction = sqlite3_value_int (argv[13]);

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
/* retrieving the BBOX */
    minx = geom->MinX;
    maxx = geom->MaxX;
    miny = geom->MinY;
    maxy = geom->MaxY;
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (rl2_get_coverage_tile_size (coverage, &tile_width, &tile_height) !=
	RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (rl2_get_coverage_srid (coverage, &srid) != RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (rl2_get_coverage_type (coverage, &sample_type, &pixel_type, &num_bands)
	!= RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    no_data = rl2_get_coverage_no_data (coverage);
    if (rl2_get_coverage_compression (coverage, &compression, &quality)
	!= RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    tilew = (double) tile_width *horz_res;
    tileh = (double) tile_height *vert_res;
    ext_x = maxx - minx;
    ext_y = maxy - miny;
    width = ext_x / horz_res;
    height = ext_y / horz_res;
    if (((double) width * horz_res) < ext_x)
	width++;
    if (((double) height * vert_res) < ext_y)
	height++;
    if (width < tile_width || width > UINT16_MAX)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (height < tile_height || height > UINT16_MAX)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }

/* SQL prepared statements */
    table = sqlite3_mprintf ("%s_sections", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (section_id, section_name, file_path, "
	 "width, height, geometry) VALUES (NULL, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO sections SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_sections", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET statistics = ? WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE sections SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_levels", cvg_name);
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
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_levl, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO levels SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tiles", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 0, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_tils, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tile_data", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, tile_data_odd, tile_data_even) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tile_data SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

/* preparing the WMS requests */
    wms_crs = sqlite3_mprintf ("EPSG:%d", srid);
    tilew = (double) tile_width *horz_res;
    tileh = (double) tile_height *vert_res;
    ext_x = maxx - minx;
    ext_y = maxy - miny;
    width = ext_x / horz_res;
    height = ext_y / horz_res;
    if ((width * horz_res) < ext_x)
	width++;
    if ((height * vert_res) < ext_y)
	height++;
    retry_list = alloc_retry_list ();
    for (y = maxy; y > miny; y -= tileh)
      {
	  for (x = minx; x < maxx; x += tilew)
	    {
		char *err_msg = NULL;
		unsigned char *rgba_tile =
		    do_wms_GetMap_get (NULL, url, proxy, wms_version, wms_layer,
				       wms_crs, swap_xy, x, y - tileh,
				       x + tilew, y, tile_width, tile_height,
				       wms_style, wms_format, opaque, 0,
				       &err_msg);
		if (rgba_tile == NULL)
		  {
		      add_retry (retry_list, x, y - tileh, x + tilew, y);
		      continue;
		  }

		params.sqlite = sqlite;
		params.rgba_tile = rgba_tile;
		params.coverage = coverage;
		params.sect_name = sect_name;
		params.x = x;
		params.y = y;
		params.width = width;
		params.height = height;
		params.tilew = tilew;
		params.tileh = tileh;
		params.srid = srid;
		params.minx = minx;
		params.miny = miny;
		params.maxx = maxx;
		params.maxy = maxy;
		params.sample_type = sample_type;
		params.num_bands = num_bands;
		params.compression = compression;
		params.horz_res = horz_res;
		params.vert_res = vert_res;
		params.tile_width = tile_width;
		params.tile_height = tile_height;
		params.no_data = no_data;
		params.stmt_sect = stmt_sect;
		params.stmt_levl = stmt_levl;
		params.stmt_tils = stmt_tils;
		params.stmt_data = stmt_data;
		if (!insert_wms_tile
		    (&params, &first, &section_stats, &section_id))
		    goto error;
	    }
      }
    for (n = 0; n < 5; n++)
      {
	  WmsRetryItemPtr retry = retry_list->first;
	  while (retry != NULL)
	    {
		char *err_msg = NULL;
		unsigned char *rgba_tile = NULL;
		if (retry->done)
		  {
		      retry = retry->next;
		      continue;
		  }
		retry->count += 1;
		rgba_tile =
		    do_wms_GetMap_get (NULL, url, proxy, wms_version, wms_layer,
				       wms_crs, swap_xy, retry->minx,
				       retry->miny, retry->maxx, retry->maxy,
				       tile_width, tile_height, wms_style,
				       wms_format, opaque, 0, &err_msg);
		if (rgba_tile == NULL)
		  {
		      retry = retry->next;
		      continue;
		  }

		params.sqlite = sqlite;
		params.rgba_tile = rgba_tile;
		params.coverage = coverage;
		params.sect_name = sect_name;
		params.x = x;
		params.y = y;
		params.width = width;
		params.height = height;
		params.tilew = tilew;
		params.tileh = tileh;
		params.srid = srid;
		params.minx = minx;
		params.miny = miny;
		params.maxx = maxx;
		params.maxy = maxy;
		params.sample_type = sample_type;
		params.num_bands = num_bands;
		params.compression = compression;
		params.horz_res = horz_res;
		params.vert_res = vert_res;
		params.tile_width = tile_width;
		params.tile_height = tile_height;
		params.no_data = no_data;
		params.stmt_sect = stmt_sect;
		params.stmt_levl = stmt_levl;
		params.stmt_tils = stmt_tils;
		params.stmt_data = stmt_data;
		if (!insert_wms_tile
		    (&params, &first, &section_stats, &section_id))
		    goto error;
		retry->done = 1;
		retry = retry->next;
	    }
      }

    if (!do_insert_stats (sqlite, section_stats, section_id, stmt_upd_sect))
	goto error;
    free_retry_list (retry_list);
    retry_list = NULL;
    sqlite3_free (wms_crs);
    wms_crs = NULL;

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

    rl2_destroy_raster_statistics (section_stats);
    section_stats = NULL;
    rl2_destroy_coverage (coverage);
    coverage = NULL;
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }

    if (rl2_update_dbms_coverage (sqlite, cvg_name) != RL2_OK)
      {
	  fprintf (stderr, "unable to update the Coverage\n");
	  goto error;
      }

    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
    return;

  error:
    if (wms_crs != NULL)
	sqlite3_free (wms_crs);
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
    if (retry_list != NULL)
	free_retry_list (retry_list);
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteGeoTiff (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double resolution)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile,
/              text compression)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile,
/              text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int worldfile = 0;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[5]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[6]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_TEXT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    blob = sqlite3_value_blob (argv[4]);
    blob_sz = sqlite3_value_bytes (argv[4]);
    if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[5]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[5]);
    if (argc > 6)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[6]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[6]);
      }
    else
	vert_res = horz_res;
    if (argc > 7)
	worldfile = sqlite3_value_int (argv[7]);
    if (argc > 8)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[8]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 9)
	tile_sz = sqlite3_value_int (argv[9]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    ret =
	rl2_export_geotiff_from_dbms (sqlite, path, coverage, horz_res,
				      vert_res, minx, miny, maxx, maxy, width,
				      height, compression, tile_sz, worldfile);
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
common_write_tiff (int with_worldfile, sqlite3_context * context, int argc,
		   sqlite3_value ** argv)
{
/* SQL function:
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double resolution)
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res)
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res, text compression)
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[5]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[6]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_TEXT)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    blob = sqlite3_value_blob (argv[4]);
    blob_sz = sqlite3_value_bytes (argv[4]);
    if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[5]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[5]);
    if (argc > 6)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[6]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[6]);
      }
    else
	vert_res = horz_res;
    if (argc > 7)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[7]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 8)
	tile_sz = sqlite3_value_int (argv[8]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (with_worldfile)
      {
	  /* TIFF + Worldfile */
	  ret =
	      rl2_export_tiff_worldfile_from_dbms (sqlite, path, coverage,
						   horz_res, vert_res, minx,
						   miny, maxx, maxy, width,
						   height, compression,
						   tile_sz);
      }
    else
      {
	  /* plain TIFF, no Worldfile */
	  ret =
	      rl2_export_tiff_from_dbms (sqlite, path, coverage, horz_res,
					 vert_res, minx, miny, maxx, maxy,
					 width, height, compression, tile_sz);
      }
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteTiffTfw (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double resolution)
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, text compression)
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (1, context, argc, argv);
}

static void
fnct_WriteTiff (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/              double vert_res, text compression)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/           double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (0, context, argc, argv);
}

static void
fnct_WriteAsciiGrid (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteAsciiGrid(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution, 
/           int is_centered)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution, 
/           int is_centered, int decimal_digits)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double resolution;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int is_centered = 1;
    int decimal_digits = 4;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[5]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    blob = sqlite3_value_blob (argv[4]);
    blob_sz = sqlite3_value_bytes (argv[4]);
    if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[5]);
	  resolution = ival;
      }
    else
	resolution = sqlite3_value_double (argv[5]);
    if (argc > 6)
	is_centered = sqlite3_value_int (argv[6]);
    if (argc > 7)
	decimal_digits = sqlite3_value_int (argv[7]);

    if (decimal_digits < 1)
	decimal_digits = 0;
    if (decimal_digits > 18)
	decimal_digits = 18;

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * resolution;
	  double ext_y = (double) height * resolution;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    ret =
	rl2_export_ascii_grid_from_dbms (sqlite, path, coverage,
					 resolution, minx,
					 miny, maxx, maxy, width,
					 height, is_centered, decimal_digits);
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_GetMapImage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetMapImage(text coverage, BLOB geom, int width, int height)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format, int transparent)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format, int transparent,
/             int quality)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format, int transparent,
/             int quality, int reaspect)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    int width;
    int height;
    int base_width;
    int base_height;
    const unsigned char *blob;
    int blob_sz;
    const char *style = "default";
    const char *format = "image/png";
    int transparent = 0;
    int quality = 80;
    int reaspect = 0;
    sqlite3 *sqlite;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    double ext_x;
    double ext_y;
    double x_res;
    double y_res;
    int level_id;
    int scale;
    int xscale;
    double xx_res;
    double yy_res;
    double aspect_org;
    double aspect_dst;
    double rescale_x;
    double rescale_y;
    int ok_style;
    int ok_format;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *image = NULL;
    int image_size;
    unsigned char *rgb = NULL;
    rl2PalettePtr palette = NULL;
    rl2PrivPalettePtr plt;
    double confidence;
    unsigned char format_id = RL2_COMPRESSION_UNKNOWN;
    unsigned char out_pixel = RL2_PIXEL_UNKNOWN;
    unsigned char out_format = RL2_COMPRESSION_UNKNOWN;
    int row;
    int col;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *rgba = NULL;
    unsigned char *gray = NULL;
    rl2GraphicsBitmapPtr base_img = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    blob = sqlite3_value_blob (argv[1]);
    blob_sz = sqlite3_value_bytes (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    if (argc > 4)
	style = (const char *) sqlite3_value_text (argv[4]);
    if (argc > 5)
	format = (const char *) sqlite3_value_text (argv[5]);
    if (argc > 6)
	transparent = sqlite3_value_int (argv[6]);
    if (argc > 7)
	quality = sqlite3_value_int (argv[7]);
    if (argc > 8)
	quality = sqlite3_value_int (argv[8]);

/* coarse args validation */
    if (width < 64 || width > 5000)
	goto error;
    if (height < 64 || height > 5000)
	goto error;
/* validating the style */
    ok_style = 0;
    if (strcmp (style, "default") == 0)
	ok_style = 1;
    if (!ok_style)
	goto error;
/* validating the format */
    ok_format = 0;
    if (strcmp (format, "image/png") == 0)
      {
	  format_id = RL2_COMPRESSION_PNG;
	  ok_format = 1;
      }
    if (strcmp (format, "image/jpeg") == 0)
      {
	  format_id = RL2_COMPRESSION_JPEG;
	  ok_format = 1;
      }
    if (!ok_format)
	goto error;

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
	goto error;
    minx = geom->MinX;
    maxx = geom->MaxX;
    miny = geom->MinY;
    maxy = geom->MaxY;
    ext_x = maxx - minx;
    ext_y = maxy - miny;
    if (ext_x <= 0.0 || ext_y <= 0.0)
	goto error;
    x_res = ext_x / (double) width;
    y_res = ext_y / (double) height;
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
	goto error;
    cvg = (rl2PrivCoveragePtr) coverage;
    out_pixel = RL2_PIXEL_UNKNOWN;
    if (cvg->sampleType == RL2_SAMPLE_UINT8 && cvg->pixelType == RL2_PIXEL_RGB
	&& cvg->nBands == 3)
	out_pixel = RL2_PIXEL_RGB;
    if (cvg->sampleType == RL2_SAMPLE_UINT8
	&& cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	out_pixel = RL2_PIXEL_GRAYSCALE;
    if (cvg->pixelType == RL2_PIXEL_PALETTE && cvg->nBands == 1)
	out_pixel = RL2_PIXEL_PALETTE;
    if (cvg->pixelType == RL2_PIXEL_MONOCHROME && cvg->nBands == 1)
	out_pixel = RL2_PIXEL_MONOCHROME;
    if (out_pixel == RL2_PIXEL_UNKNOWN)
      {
	  fprintf (stderr, "*** Unsupported Pixel !!!!\n");
	  goto error;
      }

/* retrieving the optimal resolution level */
    if (!find_best_resolution_level
	(sqlite, cvg_name, x_res, y_res, &level_id, &scale, &xscale, &xx_res,
	 &yy_res))
	goto error;
    base_width = (int) (ext_x / xx_res);
    base_height = (int) (ext_y / yy_res);
    aspect_org = (double) base_width / (double) base_height;
    aspect_dst = (double) width / (double) height;
    confidence = aspect_org / 100.0;
    if (aspect_dst >= (aspect_org - confidence)
	|| aspect_dst <= (aspect_org + confidence))
	;
    else if (aspect_org != aspect_dst && !reaspect)
	goto error;
    rescale_x = (double) width / (double) base_width;
    rescale_y = (double) height / (double) base_height;

    if (out_pixel == RL2_PIXEL_MONOCHROME)
      {
	  if (level_id != 0 && scale != 1)
	      out_pixel = RL2_PIXEL_GRAYSCALE;
      }
    if (out_pixel == RL2_PIXEL_PALETTE)
      {
	  if (level_id != 0 && scale != 1)
	      out_pixel = RL2_PIXEL_RGB;
      }

    if (rl2_get_raw_raster_data
	(sqlite, coverage, base_width, base_height, minx, miny, maxx, maxy,
	 xx_res, yy_res, &outbuf, &outbuf_size, &palette, out_pixel) != RL2_OK)
	goto error;
    if (out_pixel == RL2_PIXEL_PALETTE && palette == NULL)
	goto error;
    if (base_width == width && base_height == height)
      {
	  if (out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* converting from Monochrome to Grayscale */
		gray = malloc (base_width * base_height);
		p_in = outbuf;
		p_out = gray;
		for (row = 0; row < base_height; row++)
		  {
		      for (col = 0; col < base_width; col++)
			{
			    if (*p_in++ == 1)
				*p_out++ = 0;	/* Black */
			    else
				*p_out++ = 255;	/* White */
			}
		  }
		free (outbuf);
		outbuf = NULL;
		if (format_id == RL2_COMPRESSION_JPEG)
		  {
		      if (rl2_gray_to_jpeg
			  (base_width, base_height, gray, quality, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else if (format_id == RL2_COMPRESSION_PNG)
		  {
		      if (rl2_gray_to_png
			  (base_width, base_height, gray, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else
		    goto error;
		free (gray);
		gray = NULL;
	    }
	  else if (out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Palette */
		plt = (rl2PrivPalettePtr) palette;
		out_format = get_palette_format (plt);
		if (out_format == RL2_PIXEL_RGB)
		  {
		      /* converting from Palette to RGB */
		      rgb = malloc (base_width * base_height * 3);
		      p_in = outbuf;
		      p_out = rgb;
		      for (row = 0; row < base_height; row++)
			{
			    for (col = 0; col < base_width; col++)
			      {
				  unsigned char red = 0;
				  unsigned char green = 0;
				  unsigned char blue = 0;
				  unsigned char index = *p_in++;
				  if (index < plt->nEntries)
				    {
					rl2PrivPaletteEntryPtr entry =
					    plt->entries + index;
					red = entry->red;
					green = entry->green;
					blue = entry->blue;
				    }
				  *p_out++ = red;	/* red */
				  *p_out++ = green;	/* green */
				  *p_out++ = blue;	/* blue */
			      }
			}
		      free (outbuf);
		      outbuf = NULL;
		      if (format_id == RL2_COMPRESSION_JPEG)
			{
			    if (rl2_rgb_to_jpeg
				(base_width, base_height, rgb, quality, &image,
				 &image_size) != RL2_OK)
				goto error;
			}
		      else if (format_id == RL2_COMPRESSION_PNG)
			{
			    if (rl2_rgb_to_png
				(base_width, base_height, rgb, &image,
				 &image_size) != RL2_OK)
				goto error;
			}
		      else
			  goto error;
		      free (rgb);
		      rgb = NULL;
		  }
		else if (out_format == RL2_PIXEL_GRAYSCALE)
		  {
		      /* converting from Palette to Grayscale */
		      gray = malloc (base_width * base_height);
		      p_in = outbuf;
		      p_out = gray;
		      for (row = 0; row < base_height; row++)
			{
			    for (col = 0; col < base_width; col++)
			      {
				  unsigned char value = 0;
				  unsigned char index = *p_in++;
				  if (index < plt->nEntries)
				    {
					rl2PrivPaletteEntryPtr entry =
					    plt->entries + index;
					value = entry->red;
				    }
				  *p_out++ = value;	/* gray */
			      }
			}
		      free (outbuf);
		      outbuf = NULL;
		      if (format_id == RL2_COMPRESSION_JPEG)
			{
			    if (rl2_gray_to_jpeg
				(base_width, base_height, gray, quality, &image,
				 &image_size) != RL2_OK)
				goto error;
			}
		      else if (format_id == RL2_COMPRESSION_PNG)
			{
			    if (rl2_gray_to_png
				(base_width, base_height, gray, &image,
				 &image_size) != RL2_OK)
				goto error;
			}
		      else
			  goto error;
		  }
		else
		    goto error;
		free (gray);
		gray = NULL;
	    }
	  else if (out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		if (format_id == RL2_COMPRESSION_JPEG)
		  {
		      if (rl2_gray_to_jpeg
			  (base_width, base_height, outbuf, quality, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else if (format_id == RL2_COMPRESSION_PNG)
		  {
		      if (rl2_gray_to_png
			  (base_width, base_height, outbuf, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else
		    goto error;
	    }
	  else
	    {
		/* RGB */
		if (format_id == RL2_COMPRESSION_JPEG)
		  {
		      if (rl2_rgb_to_jpeg
			  (base_width, base_height, outbuf, quality, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else if (format_id == RL2_COMPRESSION_PNG)
		  {
		      if (rl2_rgb_to_png
			  (base_width, base_height, outbuf, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else
		    goto error;
	    }
	  sqlite3_result_blob (context, image, image_size, free);
      }
    else
      {
	  /* rescaling */
	  ctx = rl2_graph_create_context (width, height);
	  if (ctx == NULL)
	      goto error;
	  rgba = malloc (base_width * base_height * 4);
	  p_in = outbuf;
	  p_out = rgba;
	  if (out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* converting from Monochrome to Grayscale */
		for (row = 0; row < base_height; row++)
		  {
		      for (col = 0; col < base_width; col++)
			{
			    if (*p_in++ == 1)
			      {
				  *p_out++ = 0;	/* Black */
				  *p_out++ = 0;
				  *p_out++ = 0;
				  *p_out++ = 255;	/* alpha */
			      }
			    else
			      {
				  *p_out++ = 255;	/* White */
				  *p_out++ = 255;
				  *p_out++ = 255;
				  *p_out++ = 255;	/* alpha */
			      }
			}
		  }
		free (outbuf);
		outbuf = NULL;
	    }
	  else if (out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Palette */
		plt = (rl2PrivPalettePtr) palette;
		out_format = get_palette_format (plt);
		if (out_format == RL2_PIXEL_RGB)
		  {
		      /* converting from Palette to RGB */
		      for (row = 0; row < base_height; row++)
			{
			    for (col = 0; col < base_width; col++)
			      {
				  unsigned char red = 0;
				  unsigned char green = 0;
				  unsigned char blue = 0;
				  unsigned char index = *p_in++;
				  if (index < plt->nEntries)
				    {
					rl2PrivPaletteEntryPtr entry =
					    plt->entries + index;
					red = entry->red;
					green = entry->green;
					blue = entry->blue;
				    }
				  *p_out++ = red;	/* red */
				  *p_out++ = green;	/* green */
				  *p_out++ = blue;	/* blue */
				  *p_out++ = 255;	/* alpha */
			      }
			}
		      free (outbuf);
		      outbuf = NULL;
		  }
		else if (out_format == RL2_PIXEL_GRAYSCALE)
		  {
		      /* converting from Palette to Grayscale */
		      for (row = 0; row < base_height; row++)
			{
			    for (col = 0; col < base_width; col++)
			      {
				  unsigned char value = 0;
				  unsigned char index = *p_in++;
				  if (index < plt->nEntries)
				    {
					rl2PrivPaletteEntryPtr entry =
					    plt->entries + index;
					value = entry->red;
				    }
				  *p_out++ = value;	/* red */
				  *p_out++ = value;	/* green */
				  *p_out++ = value;	/* blue */
				  *p_out++ = 255;	/* alpha */
			      }
			}
		      free (outbuf);
		      outbuf = NULL;
		  }
		else
		    goto error;
	    }
	  else if (out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		for (row = 0; row < base_height; row++)
		  {
		      for (col = 0; col < base_width; col++)
			{
			    unsigned char gray = *p_in++;
			    *p_out++ = gray;	/* red */
			    *p_out++ = gray;	/* green */
			    *p_out++ = gray;	/* blue */
			    *p_out++ = 255;	/* alpha */
			}
		  }
	    }
	  else
	    {
		/* RGB */
		for (row = 0; row < base_height; row++)
		  {
		      for (col = 0; col < base_width; col++)
			{
			    *p_out++ = *p_in++;	/* red */
			    *p_out++ = *p_in++;	/* green */
			    *p_out++ = *p_in++;	/* blue */
			    *p_out++ = 255;	/* alpha */
			}
		  }
	    }
	  free (outbuf);
	  outbuf = NULL;
	  base_img = rl2_graph_create_bitmap (rgba, base_width, base_height);
	  if (base_img == NULL)
	      goto error;
	  rgba = NULL;
	  rl2_graph_draw_rescaled_bitmap (ctx, base_img, rescale_x, rescale_y,
					  0, 0);
	  rl2_graph_destroy_bitmap (base_img);
	  rgb = rl2_graph_get_context_rgb_array (ctx);
	  rl2_graph_destroy_context (ctx);
	  if (rgb == NULL)
	      goto error;
	  if (out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		unsigned char *gray = malloc (width * height);
		p_in = rgb;
		p_out = gray;
		for (row = 0; row < height; row++)
		  {
		      for (col = 0; col < width; col++)
			{
			    *p_out++ = *p_in++;
			    p_in += 2;
			}
		  }
		free (rgb);
		rgb = NULL;
		if (format_id == RL2_COMPRESSION_JPEG)
		  {
		      if (rl2_gray_to_jpeg
			  (width, height, gray, quality, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else if (format_id == RL2_COMPRESSION_PNG)
		  {
		      if (rl2_gray_to_png
			  (width, height, gray, &image, &image_size) != RL2_OK)
			  goto error;
		  }
		else
		    goto error;
		free (gray);
		gray = NULL;
	    }
	  else
	    {
		/* RGB */
		if (format_id == RL2_COMPRESSION_JPEG)
		  {
		      if (rl2_rgb_to_jpeg
			  (width, height, rgb, quality, &image,
			   &image_size) != RL2_OK)
			  goto error;
		  }
		else if (format_id == RL2_COMPRESSION_PNG)
		  {
		      if (rl2_rgb_to_png
			  (width, height, rgb, &image, &image_size) != RL2_OK)
			  goto error;
		  }
		else
		    goto error;
		free (rgb);
		rgb = NULL;
	    }
	  sqlite3_result_blob (context, image, image_size, free);
      }


    rl2_destroy_coverage (coverage);
    if (outbuf != NULL)
	free (outbuf);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (outbuf != NULL)
	free (outbuf);
    if (rgb != NULL)
	free (rgb);
    if (rgba != NULL)
	free (rgba);
    if (gray != NULL)
	free (gray);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    sqlite3_result_null (context);
}

static void
register_rl2_sql_functions (void *p_db)
{
    sqlite3 *db = p_db;
    const char *security_level;
    sqlite3_create_function (db, "rl2_version", 0, SQLITE_ANY, 0,
			     fnct_rl2_version, 0, 0);
    sqlite3_create_function (db, "rl2_target_cpu", 0, SQLITE_ANY, 0,
			     fnct_rl2_target_cpu, 0, 0);
    sqlite3_create_function (db, "IsValidPixel", 3, SQLITE_ANY, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidPixel", 3, SQLITE_ANY, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "IsValidRasterPalette", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterPalette", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 2, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterStatistics", 2, SQLITE_ANY,
			     0, fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 3, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterStatistics", 3, SQLITE_ANY,
			     0, fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterTile", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterTile", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 10, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 11, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 12, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 10, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 11, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 12, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "DeleteSection", 2, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "RL2_DeleteSection", 2, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "DeleteSection", 3, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "RL2_DeleteSection", 3, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "DropCoverage", 1, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_DropCoverage", 1, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "DropCoverage", 2, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_DropCoverage", 2, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "CreatePixel", 3, SQLITE_ANY, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "RL2_CreatePixel", 3, SQLITE_ANY, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "GetPixelType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "GetPixelSampleType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelSampleType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "GetPixelNumBands", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelNumBands", 1, SQLITE_ANY, 0,
			     fnct_GetPixelNumBands, 0, 0);
    sqlite3_create_function (db, "GetPixelValue", 2, SQLITE_ANY, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelValue", 2, SQLITE_ANY, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "SetPixelValue", 3, SQLITE_ANY, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_SetPixelValue", 3, SQLITE_ANY, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "IsTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "IsOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "SetTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "SetOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "PixelEquals", 2, SQLITE_ANY, 0,
			     fnct_PixelEquals, 0, 0);
    sqlite3_create_function (db, "RL2_PixelEquals", 2, SQLITE_ANY, 0,
			     fnct_PixelEquals, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 1, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 1, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 2, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 2, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 3, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 3, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 4, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 4, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 1, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 1, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 2, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 2, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 3, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 3, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 4, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 4, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 5, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 5, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 6, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 6, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 7, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 7, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 8, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 8, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 9, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 9, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);

/*
// enabling ImportRaster and ExportRaster
//
// these functions could potentially introduce serious security issues,
// most notably when invoked from within some Trigger
// - BlobToFile: some arbitrary code, possibly harmful (e.g. virus or 
//   trojan) could be installed on the local file-system, the user being
//   completely unaware of this
// - ImportRaster: some file could be maliciously "stolen" from the local
//   file system and then inserted into the DB
// - ExportRaster could eventually install some malicious payload into the
//   local filesystem; or could potentially flood the local file-system by
//   outputting a huge size of data
//
// so by default such functions are disabled.
// if for any good/legitimate reason the user really wants to enable them
// the following environment variable has to be explicitly declared:
//
// SPATIALITE_SECURITY=relaxed
//
*/
    security_level = getenv ("SPATIALITE_SECURITY");
    if (security_level == NULL)
	;
    else if (strcasecmp (security_level, "relaxed") == 0)
      {
	  sqlite3_create_function (db, "LoadRaster", 2, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 3, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 4, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 5, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 6, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 2, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 3, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 4, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 5, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 6, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 2, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 3, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 4, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 5, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 6, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 7, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 2, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 3, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 4, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 5, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 6, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 7, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 9, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 9, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 10, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 10, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 11, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 11, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 12, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 12, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 13, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 13, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 14, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 14, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 10, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 10, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 6, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 6, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 7, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 7, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 8, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 8, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 9, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 9, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 6, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 6, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 7, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 7, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 8, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 8, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
      }
}

static void
rl2_splash_screen (int verbose)
{
    if (isatty (1))
      {
	  /* printing "hello" message only when stdout is on console */
	  if (verbose)
	    {
		printf ("RasterLite-2 version : %s\n", rl2_version ());
		printf ("TARGET CPU ..........: %s\n", rl2_target_cpu ());
	    }
      }
}

#ifndef LOADABLE_EXTENSION

RL2_DECLARE void
rl2_init (sqlite3 * handle, int verbose)
{
/* used when SQLite initializes as an ordinary lib */
    register_rl2_sql_functions (handle);
    rl2_splash_screen (verbose);
}

#else /* built as LOADABLE EXTENSION only */

SQLITE_EXTENSION_INIT1 static int
init_rl2_extension (sqlite3 * db, char **pzErrMsg,
		    const sqlite3_api_routines * pApi)
{
    SQLITE_EXTENSION_INIT2 (pApi);

    register_rl2_sql_functions (db);
    return 0;
}

#if !(defined _WIN32) || defined(__MINGW32__)
/* MSVC is unable to understand this declaration */
__attribute__ ((visibility ("default")))
#endif
     RL2_DECLARE int
	 sqlite3_modrasterlite_init (sqlite3 * db, char **pzErrMsg,
				     const sqlite3_api_routines * pApi)
{
/* SQLite invokes this routine once when it dynamically loads the extension. */
    return init_rl2_extension (db, pzErrMsg, pApi);
}

#endif
