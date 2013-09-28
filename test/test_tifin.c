/*

 test_tifin.c -- RasterLite2 Test Case

 Author: Alessandro Furieri <a.furieri@lqt.it>

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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "rasterlite2/rasterlite2.h"

static int
test_tiff (const char *name, unsigned char sample_type,
	   unsigned char pixel_type, unsigned char nBands)
{
/* testing a TIFF source */
    rl2TiffOriginPtr origin = NULL;
    rl2CoveragePtr coverage = NULL;
    rl2RasterPtr raster = NULL;
    rl2SectionPtr img = NULL;
    int row;
    int col;
    unsigned short width;
    unsigned short height;
    char tile_name[256];
    char path[1024];

    coverage =
	rl2_create_coverage (name, sample_type, pixel_type, nBands,
			     RL2_COMPRESSION_PNG, 0, 432, 432, NULL);
    if (coverage == NULL)
      {
	  fprintf (stderr, "ERROR: unable to create the Coverage\n");
	  return -1;
      }

    sprintf (path, "./%s.tif", name);
    origin = rl2_create_tiff_origin (path);
    if (origin == NULL)
      {
	  fprintf (stderr, "ERROR: unable to open %s\n", path);
	  return -2;
      }

    if (rl2_eval_tiff_origin_compatibility (coverage, origin) != RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch: %s\n", name);
	  return -3;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 1, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startRow mismatch\n");
	  return -4;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 0, 1) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startCol mismatch\n");
	  return -5;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 22000, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startRow too big\n");
	  return -6;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 0, 22000) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startCol too big\n");
	  return -7;
      }

    if (rl2_get_tiff_origin_size (origin, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "Error - TIFF size\n");
	  return -8;
      }

    for (row = 0; row < height; row += 432)
      {
	  for (col = 0; col < width; col += 432)
	    {
		sprintf (tile_name, "%s_%04d_%04d", name, row, col);
		raster =
		    rl2_get_tile_from_tiff_origin (coverage, origin, row, col);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "ERROR: unable to get tile [Row=%d Col=%d] from %s\n",
			       row, col, name);
		      return -9;
		  }
		img =
		    rl2_create_section (name, RL2_COMPRESSION_NONE,
					RL2_TILESIZE_UNDEFINED,
					RL2_TILESIZE_UNDEFINED, raster);
		if (img == NULL)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "ERROR: unable to create a Section [%s]\n",
			       name);
		      return -10;
		  }
		sprintf (path, "./%s.png", tile_name);
		if (rl2_section_to_png (img, path) != RL2_OK)
		  {
		      fprintf (stderr, "Unable to write: %s\n", path);
		      return -11;
		  }
		rl2_destroy_section (img);
		unlink (path);
	    }
      }

    rl2_destroy_coverage (coverage);
    rl2_destroy_tiff_origin (origin);
    return 0;
}

int
main (int argc, char *argv[])
{
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (test_tiff ("gray-striped", RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1) !=
	0)
	return -1;
    if (test_tiff ("gray-tiled", RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1) != 0)
	return -2;
    if (test_tiff ("plt-striped", RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1) != 0)
	return -3;
    if (test_tiff ("plt-tiled", RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1) != 0)
	return -4;
    if (test_tiff ("rgb-striped", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3) != 0)
	return -5;
    if (test_tiff ("rgb-tiled", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3) != 0)
	return -6;
    if (test_tiff ("mono3s", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -7;
    if (test_tiff ("mono4s", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -8;
    if (test_tiff ("mono3t", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -9;
    if (test_tiff ("mono4t", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -10;

    return 0;
}
