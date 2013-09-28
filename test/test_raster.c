/*

 test_raster.c -- RasterLite2 Test Case

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
#include <memory.h>
#include <float.h>

#include "rasterlite2/rasterlite2.h"

static rl2PalettePtr
build_palette (int num)
{
    int i;
    rl2PalettePtr plt = rl2_create_palette (num);
    for (i = 0; i < num; i++)
	rl2_set_palette_color (plt, i, 255 - i, 255 - i, 255 - i, 255);
    return plt;
}

int
main (int argc, char *argv[])
{
    rl2PalettePtr palette;
    rl2RasterPtr raster;
    rl2PixelPtr no_data;
    rl2PixelPtr pixel;
    rl2SectionPtr img;
    unsigned char sample;
    unsigned char *mask;
    unsigned char *bufpix = malloc (256 * 256);
    memset (bufpix, 255, 256 * 256);

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */


    if (rl2_create_raster (0, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 0 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - ZERO width\n");
	  return -1;
      }

    if (rl2_create_raster (256, 0, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 0, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - ZERO height\n");
	  return -2;
      }

    if (rl2_create_raster (256, 256, 0xff, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - invalid sample\n");
	  return -3;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, 0xff, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - invalid pixel\n");
	  return -4;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   NULL, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - NULL pixmap\n");
	  return -5;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 255 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - mismatching pixmap size\n");
	  return -6;
      }

    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/256\n");
	  return -7;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample GRAYSCALE/256r\n");
	  return -8;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected raster pixel GRAYSCALE/256\n");
	  return -9;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRAYSCALE/256\n");
	  return -10;
      }

    if (rl2_get_raster_width (raster) != 256)
      {
	  fprintf (stderr, "Unexpected raster tile width\n");
	  return -11;
      }

    if (rl2_get_raster_height (raster) != 256)
      {
	  fprintf (stderr, "Unexpected raster tile height\n");
	  return -12;
      }

    if (rl2_raster_georeference_center (raster, 4326, 0.5, 0.5, 11.5, 42.5) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to georeference a raster (Center point)\n");
	  return -13;
      }

    if (rl2_get_raster_srid (raster) != 4326)
      {
	  fprintf (stderr, "Unexpected raster SRID\n");
	  return -14;
      }

    if (rl2_get_raster_minX (raster) != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (Center point)\n");
	  return -15;
      }

    if (rl2_get_raster_minY (raster) != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (Center point)\n");
	  return -16;
      }

    if (rl2_get_raster_maxX (raster) != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (Center point)\n");
	  return -17;
      }

    if (rl2_get_raster_maxY (raster) != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (Center point)\n");
	  return -18;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    memset (bufpix, 0, 256 * 256);
    memset (bufpix + (256 * 128), 1, 256);
    memset (bufpix + (256 * 129), 1, 256);
    memset (bufpix + (256 * 130), 1, 256);
    memset (bufpix + (256 * 131), 1, 256);
    memset (bufpix + (256 * 136), 1, 256);
    memset (bufpix + (256 * 137), 1, 256);
    memset (bufpix + (256 * 138), 1, 256);
    memset (bufpix + (256 * 139), 1, 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a MONOCHROME raster\n");
	  return -19;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample MONOCHROME\n");
	  return -20;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_MONOCHROME)
      {
	  fprintf (stderr, "Unexpected raster pixel MONOCHROME\n");
	  return -21;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands MONOCHROME\n");
	  return -22;
      }

    if (rl2_raster_georeference_upper_left
	(raster, 4326, 0.5, 0.5, -52.5, 106.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (UpperLeft corner)\n");
	  return -23;
      }

    if (rl2_get_raster_minX (raster) != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (UpperLeft corner)\n");
	  return -24;
      }

    if (rl2_get_raster_minY (raster) != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (UpperLeft corner)\n");
	  return -25;
      }

    if (rl2_get_raster_maxX (raster) != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (UpperLeft corner)\n");
	  return -26;
      }

    if (rl2_get_raster_maxY (raster) != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (UpperLeft corner)\n");
	  return -27;
      }

    img = rl2_create_section ("mono", RL2_COMPRESSION_NONE, 256, 256, raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Monochrome image\n");
	  return -28;
      }
    if (rl2_section_to_gif (img, "./raster_mono.gif") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_mono.gif\n");
	  return -29;
      }
    unlink ("./raster_mono.gif");

    if (rl2_section_to_png (img, "./raster_mono.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_mono.png\n");
	  return -30;
      }
    unlink ("./raster_mono.png");

    if (rl2_section_to_jpeg (img, "./raster_mono.jpg", 60) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_mono.jpg\n");
	  return -40;
      }
    unlink ("./raster_mono.jpg");

    rl2_destroy_section (img);

    bufpix = malloc (256 * 256);
    memset (bufpix, 3, 256 * 256);
    memset (bufpix + (256 * 120), 2, 256);
    memset (bufpix + (256 * 121), 2, 256);
    memset (bufpix + (256 * 122), 2, 256);
    memset (bufpix + (256 * 123), 2, 256);
    memset (bufpix + (256 * 124), 1, 256);
    memset (bufpix + (256 * 125), 1, 256);
    memset (bufpix + (256 * 126), 1, 256);
    memset (bufpix + (256 * 127), 1, 256);
    memset (bufpix + (256 * 128), 0, 256);
    memset (bufpix + (256 * 129), 0, 256);
    memset (bufpix + (256 * 130), 0, 256);
    memset (bufpix + (256 * 131), 0, 256);
    memset (bufpix + (256 * 132), 1, 256);
    memset (bufpix + (256 * 133), 1, 256);
    memset (bufpix + (256 * 134), 1, 256);
    memset (bufpix + (256 * 135), 1, 256);
    memset (bufpix + (256 * 136), 2, 256);
    memset (bufpix + (256 * 137), 2, 256);
    memset (bufpix + (256 * 138), 2, 256);
    memset (bufpix + (256 * 139), 2, 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/4 raster\n");
	  return -41;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample GRAYSCALE/4\n");
	  return -42;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected raster pixel GRAYSCALE/4\n");
	  return -43;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRAYSCALE/4\n");
	  return -44;
      }

    if (rl2_raster_georeference_lower_left
	(raster, 4326, 0.5, 0.5, -52.5, -21.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (LowerLeft corner)\n");
	  return -45;
      }

    if (rl2_get_raster_minX (raster) != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (LowerLeft corner)\n");
	  return -46;
      }

    if (rl2_get_raster_minY (raster) != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (LowerLeft corner)\n");
	  return -47;
      }

    if (rl2_get_raster_maxX (raster) != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (LowerLeft corner)\n");
	  return -48;
      }

    if (rl2_get_raster_maxY (raster) != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (LowerLeft corner)\n");
	  return -49;
      }

    img = rl2_create_section ("gray4", RL2_COMPRESSION_NONE, 256, 256, raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to creat a Grayscale/4 image\n");
	  return -50;
      }

    if (rl2_section_to_gif (img, "./raster_gray4.gif") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray4.gif\n");
	  return -51;
      }
    unlink ("./raster_gray4.gif");

    if (rl2_section_to_png (img, "./raster_gray4.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray4.png\n");
	  return -52;
      }
    unlink ("./raster_gray4.png");

    if (rl2_section_to_jpeg (img, "./raster_gray4.jpg", 60) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray4.jpg\n");
	  return -53;
      }
    unlink ("./raster_gray4.jpg");

    rl2_destroy_section (img);

    bufpix = malloc (256 * 256);
    memset (bufpix, 15, 256 * 256);
    memset (bufpix + (256 * 120), 0, 256);
    memset (bufpix + (256 * 122), 0, 256);
    memset (bufpix + (256 * 123), 1, 256);
    memset (bufpix + (256 * 124), 2, 256);
    memset (bufpix + (256 * 125), 3, 256);
    memset (bufpix + (256 * 126), 4, 256);
    memset (bufpix + (256 * 127), 5, 256);
    memset (bufpix + (256 * 128), 6, 256);
    memset (bufpix + (256 * 129), 7, 256);
    memset (bufpix + (256 * 130), 8, 256);
    memset (bufpix + (256 * 131), 9, 256);
    memset (bufpix + (256 * 132), 10, 256);
    memset (bufpix + (256 * 133), 10, 256);
    memset (bufpix + (256 * 134), 12, 256);
    memset (bufpix + (256 * 135), 13, 256);
    memset (bufpix + (256 * 136), 14, 256);
    memset (bufpix + (256 * 137), 15, 256);
    memset (bufpix + (256 * 139), 0, 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/16 raster\n");
	  return -54;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample GRAYSCALE/16\n");
	  return -55;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected raster pixel GRAYSCALE/16\n");
	  return -56;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRAYSCALE/16\n");
	  return -57;
      }

    if (rl2_raster_georeference_lower_right
	(raster, 4326, 0.5, 0.5, 75.5, -21.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (LowerRight corner)\n");
	  return -58;
      }

    if (rl2_get_raster_minX (raster) != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (LowerRight corner)\n");
	  return -59;
      }

    if (rl2_get_raster_minY (raster) != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (LowerRight corner)\n");
	  return -60;
      }

    if (rl2_get_raster_maxX (raster) != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (LowerRight corner)\n");
	  return -61;
      }

    if (rl2_get_raster_maxY (raster) != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (LowerRight corner)\n");
	  return -62;
      }

    img = rl2_create_section ("gray16", RL2_COMPRESSION_NONE, 256, 256, raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to creat a Grayscale/16 image\n");
	  return -63;
      }

    if (rl2_section_to_gif (img, "./raster_gray16.gif") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray16.gif\n");
	  return -64;
      }
    unlink ("./raster_gray16.gif");

    if (rl2_section_to_png (img, "./raster_gray16.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray16.png\n");
	  return -65;
      }
    unlink ("./raster_gray16.png");

    if (rl2_section_to_jpeg (img, "./raster_gray16.jpg", 60) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray16.jpg\n");
	  return -66;
      }
    unlink ("./raster_gray16.jpg");

    rl2_destroy_section (img);

    bufpix = malloc (3 * 256 * 256);
    memset (bufpix, 255, 3 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   bufpix, 3 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a RGB raster\n");
	  return -67;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample RGB\n");
	  return -68;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_RGB)
      {
	  fprintf (stderr, "Unexpected raster pixel RGB\n");
	  return -69;
      }

    if (rl2_get_raster_bands (raster) != 3)
      {
	  fprintf (stderr, "Unexpected raster # bands RGB\n");
	  return -70;
      }

    if (rl2_raster_georeference_upper_right
	(raster, 4326, 0.5, 0.5, 75.5, 106.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (UpperRight corner)\n");
	  return -71;
      }

    if (rl2_get_raster_minX (raster) != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (UpperRight corner)\n");
	  return -72;
      }

    if (rl2_get_raster_minY (raster) != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (UpperRight corner)\n");
	  return -73;
      }

    if (rl2_get_raster_maxX (raster) != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (UpperRight corner)\n");
	  return -74;
      }

    if (rl2_get_raster_maxY (raster) != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (UpperRight corner)\n");
	  return -75;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (2 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 2 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/U16 raster\n");
	  return -77;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT16)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/U16\n");
	  return -78;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/U16\n");
	  return -79;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/U16\n");
	  return -80;
      }

    if (rl2_raster_georeference_frame (raster, 4326, -52.5, -21.5, 75.5, 106.5)
	!= RL2_OK)
      {
	  fprintf (stderr, "Unable to georeference a raster (frame)\n");
	  return -81;
      }

    if (rl2_raster_georeference_frame (raster, 4326, 82.5, -21.5, 75.5, 106.5)
	!= RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected: georeference a raster (invalid frame)\n");
	  return -581;
      }

    if (rl2_raster_georeference_frame (raster, 4326, -52.5, 121.5, 75.5, 106.5)
	!= RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected: georeference a raster (invalid frame)\n");
	  return -681;
      }

    if (rl2_get_raster_horizontal_resolution (raster) != 0.5)
      {
	  fprintf (stderr, "Unexpected raster horizontal resolution (frame)\n");
	  return -82;
      }

    if (rl2_get_raster_vertical_resolution (raster) != 0.5)
      {
	  fprintf (stderr, "Unexpected raster vertical resolution (frame)\n");
	  return -83;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (4 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_INT32, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 4 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/32 raster\n");
	  return -84;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_INT32)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/32\n");
	  return -85;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/32\n");
	  return -86;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/32\n");
	  return -87;
      }

    if (rl2_get_raster_horizontal_resolution (raster) != DBL_MAX)
      {
	  fprintf (stderr,
		   "Unexpected raster horizontal resolution (GRID/32)\n");
	  return -88;
      }

    if (rl2_get_raster_vertical_resolution (raster) != DBL_MAX)
      {
	  fprintf (stderr, "Unexpected raster vertical resolution (GRID/32)\n");
	  return -89;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (2 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_INT16, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 2 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/16 raster\n");
	  return -90;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_INT16)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/16\n");
	  return -91;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/16\n");
	  return -92;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/16\n");
	  return -93;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/8 raster\n");
	  return -94;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_INT8)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/8\n");
	  return -95;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/8\n");
	  return -96;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/8\n");
	  return -97;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/U8 raster\n");
	  return -98;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/U8\n");
	  return -99;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/U8\n");
	  return -100;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/U8\n");
	  return -101;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (4 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT32, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 4 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/U32 raster\n");
	  return -102;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT32)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/U32\n");
	  return -103;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/U32\n");
	  return -104;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/U32\n");
	  return -105;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (4 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 4 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/FLT raster\n");
	  return -106;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_FLOAT)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/FLT\n");
	  return -107;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/FLT\n");
	  return -108;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/FLT\n");
	  return -109;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (8 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 8 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/DBL raster\n");
	  return -110;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_DOUBLE)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/DBL\n");
	  return -111;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/DBL\n");
	  return -112;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/DBL\n");
	  return -113;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (2);
    bufpix = malloc (256 * 256);
    memset (bufpix, 1, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/1 raster\n");
	  return -114;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/1\n");
	  return -115;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/1\n");
	  return -116;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/1\n");
	  return -117;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (4);
    bufpix = malloc (256 * 256);
    memset (bufpix, 3, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/2 raster\n");
	  return -118;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/2\n");
	  return -119;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/2\n");
	  return -120;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/2\n");
	  return -121;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (16);
    bufpix = malloc (256 * 256);
    memset (bufpix, 15, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/4 raster\n");
	  return -122;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/4\n");
	  return -123;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/4\n");
	  return -124;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/4\n");
	  return -125;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (256);
    bufpix = malloc (256 * 256);
    memset (bufpix, 255, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/8 raster\n");
	  return -126;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/8\n");
	  return -127;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/8\n");
	  return -128;
      }

    if (rl2_get_raster_bands (raster) != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/8\n");
	  return -129;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (5 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 5,
			   bufpix, 5 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a MULTIBAND/5 raster\n");
	  return -130;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample MULTIBAND/5\n");
	  return -131;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_MULTIBAND)
      {
	  fprintf (stderr, "Unexpected raster pixel MULTIBAND/5\n");
	  return -132;
      }

    if (rl2_get_raster_bands (raster) != 5)
      {
	  fprintf (stderr, "Unexpected raster # bands MULTIBAND/5\n");
	  return -133;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (6 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 3,
			   bufpix, 6 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a MULTIBAND/16 raster\n");
	  return -134;
      }

    if (rl2_get_raster_sample_type (raster) != RL2_SAMPLE_UINT16)
      {
	  fprintf (stderr, "Unexpected raster sample MULTIBAND/16\n");
	  return -135;
      }

    if (rl2_get_raster_pixel_type (raster) != RL2_PIXEL_MULTIBAND)
      {
	  fprintf (stderr, "Unexpected raster pixel MULTIBAND/16\n");
	  return -136;
      }

    if (rl2_get_raster_bands (raster) != 3)
      {
	  fprintf (stderr, "Unexpected raster # bands MULTIBAND/16\n");
	  return -137;
      }
    rl2_destroy_raster (raster);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -138;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Red NoData #1\n");
	  return -139;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Green NoData #1\n");
	  return -140;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Blue NoData #1\n");
	  return -141;
      }

    bufpix = malloc (256 * 256 * 3);
    memset (bufpix, 255, 256 * 256 * 3);

    raster = rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
				bufpix, 256 * 256 * 3, NULL, NULL, 0, no_data);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a valid raster\n");
	  return -142;
      }

    no_data = rl2_get_raster_no_data (raster);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to get NoData from raster #1\n");
	  return -143;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_RED_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RED NoData from raster #1\n");
	  return -144;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected RED NoData from raster #1\n");
	  return -145;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GREEN NoData from raster #1\n");
	  return -146;
      }

    if (sample != 255)
      {
	  fprintf (stderr, "Unexpected GREEN NoData from raster #1\n");
	  return -147;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BLUE NoData from raster #1\n");
	  return -148;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected BLUE NoData from raster #1\n");
	  return -149;
      }

    no_data = rl2_create_raster_pixel (raster);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a pixel for raster #1\n");
	  return -150;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Red NoData #2\n");
	  return -151;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Green NoData #2\n");
	  return -152;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Blue NoData #2\n");
	  return -153;
      }

    no_data = rl2_get_raster_no_data (raster);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to get NoData from raster #2\n");
	  return -157;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_RED_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RED NoData from raster #2\n");
	  return -158;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected RED NoData from raster #2\n");
	  return -159;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GREEN NoData from raster #2\n");
	  return -160;
      }

    if (sample != 255)
      {
	  fprintf (stderr, "Unexpected GREEN NoData from raster #2\n");
	  return -161;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BLUE NoData from raster #2\n");
	  return -162;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected BLUE NoData from raster #2\n");
	  return -163;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -164;
      }

    bufpix = malloc (256 * 256 * 3);
    memset (bufpix, 255, 256 * 256 * 3);

    if (rl2_create_raster (1024, 1024, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0,
			   no_data) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create raster (invalid NoData)\n");
	  return -166;
      }
    free (bufpix);

    rl2_destroy_pixel (no_data);

    rl2_destroy_raster (raster);

    if (rl2_get_raster_no_data (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: get NoData (NULL raster)\n");
	  return -167;
      }

    if (rl2_create_raster_pixel (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: get pixel (NULL raster)\n");
	  return -168;
      }

    bufpix = malloc (256 * 256);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unable to create a MONOCHROME raster\n");
	  return -170;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL))
      {
	  fprintf (stderr, "Unexpected result: create a MONOCHROME raster\n");
	  return -171;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL))
      {
	  fprintf (stderr, "Unexpected result: create a MONOCHROME raster\n");
	  return -172;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_INT8, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MONOCHROME raster\n");
	  return -173;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a GRAYSCALE raster\n");
	  return -174;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_INT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a GRAYSCALE raster\n");
	  return -175;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a DATAGRID raster\n");
	  return -176;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a DATAGRID raster\n");
	  return -177;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_MULTIBAND, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -178;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -179;
      }
    free (bufpix);

    bufpix = malloc (256 * 256 * 3);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_FLOAT, RL2_PIXEL_MULTIBAND, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -180;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_FLOAT, RL2_PIXEL_RGB, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -181;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 2,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -182;
      }
    free (bufpix);

    bufpix = malloc (256 * 256 * 2);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 2,
			   bufpix, 256 * 256 * 2, NULL, NULL, 0, NULL);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (2 Bands)\n");
	  return -183;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 2);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -184;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, no_data) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster with NoData\n");
	  return -186;
      }

    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -187;
      }

    rl2_destroy_pixel (no_data);
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    mask = malloc (256 * 256);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a MONOCHROME raster with mask\n");
	  return -189;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster with mask\n");
	  return -190;
      }

    palette = build_palette (2);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Palette raster with mask\n");
	  return -191;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Grayscale raster with palette\n");
	  return -192;
      }

    *bufpix = 2;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Palette raster [invalid buffer]\n");
	  return -193;
      }
    rl2_destroy_palette (palette);

    *bufpix = 2;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a MONOCHROME raster [invalid buffer]\n");
	  return -194;
      }

    *bufpix = 4;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/4 raster [invalid buffer]\n");
	  return -195;
      }

    *bufpix = 16;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster [invalid buffer]\n");
	  return -196;
      }

    *mask = 2;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster [invalid mask]\n");
	  return -197;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, mask, 255 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster with mask\n");
	  return -198;
      }
    free (bufpix);
    free (mask);

    if (rl2_get_raster_sample_type (NULL) != RL2_SAMPLE_UNKNOWN)
      {
	  fprintf (stderr, "Unexpected result: Raster Sample (NULL)\n");
	  return -199;
      }

    if (rl2_get_raster_pixel_type (NULL) != RL2_PIXEL_UNKNOWN)
      {
	  fprintf (stderr, "Unexpected result: Raster Pixel (NULL)\n");
	  return -200;
      }

    if (rl2_get_raster_bands (NULL) != RL2_BANDS_UNKNOWN)
      {
	  fprintf (stderr, "Unexpected result: Raster Bands (NULL)\n");
	  return -201;
      }

    if (rl2_get_raster_width (NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster Width (NULL)\n");
	  return -202;
      }

    if (rl2_get_raster_height (NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster Height (NULL)\n");
	  return -203;
      }

    if (rl2_get_raster_srid (NULL) != RL2_GEOREFERENCING_NONE)
      {
	  fprintf (stderr, "Unexpected result: Raster SRID (NULL)\n");
	  return -204;
      }

    if (rl2_get_raster_minX (NULL) != DBL_MAX)
      {
	  fprintf (stderr, "Unexpected result: Raster MinX (NULL)\n");
	  return -205;
      }

    if (rl2_get_raster_minY (NULL) != DBL_MAX)
      {
	  fprintf (stderr, "Unexpected result: Raster MinY (NULL)\n");
	  return -206;
      }

    if (rl2_get_raster_maxX (NULL) != DBL_MAX)
      {
	  fprintf (stderr, "Unexpected result: Raster MaxX (NULL)\n");
	  return -207;
      }

    if (rl2_get_raster_maxY (NULL) != DBL_MAX)
      {
	  fprintf (stderr, "Unexpected result: Raster MaxY (NULL)\n");
	  return -208;
      }

    if (rl2_get_raster_horizontal_resolution (NULL) != DBL_MAX)
      {
	  fprintf (stderr, "Unexpected result: Raster HorzRes (NULL)\n");
	  return -209;
      }

    if (rl2_get_raster_vertical_resolution (NULL) != DBL_MAX)
      {
	  fprintf (stderr, "Unexpected result: Raster VertRes (NULL)\n");
	  return -210;
      }

    if (rl2_get_raster_palette (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Raster Palette (NULL)\n");
	  return -211;
      }

    if (rl2_raster_georeference_center (NULL, 4326, 1, 1, 1, 1) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster GeoRef Center (NULL)\n");
	  return -212;
      }

    if (rl2_raster_georeference_upper_left (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef UpperLeft (NULL)\n");
	  return -213;
      }

    if (rl2_raster_georeference_upper_right (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef UpperRight (NULL)\n");
	  return -213;
      }

    if (rl2_raster_georeference_lower_left (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef LowerLeft (NULL)\n");
	  return -214;
      }

    if (rl2_raster_georeference_lower_right (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef LowerRight (NULL)\n");
	  return -215;
      }

    if (rl2_raster_georeference_frame (NULL, 4326, 1, 1, 1, 1) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster GeoRef Frame (NULL)\n");
	  return -215;
      }

    rl2_destroy_raster (NULL);

    bufpix = malloc (256 * 256);
    memset (bufpix, 255, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/256\n");
	  return -216;
      }

    pixel = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create UINT16 pixels for testing\n");
	  return -217;
      }

    if (rl2_get_raster_pixel (NULL, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (NULL, NULL)\n");
	  return -218;
      }

    if (rl2_set_raster_pixel (NULL, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (NULL, NULL)\n");
	  return -218;
      }

    if (rl2_get_raster_pixel (raster, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (valid, NULL)\n");
	  return -219;
      }

    if (rl2_set_raster_pixel (raster, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (valid, NULL)\n");
	  return -220;
      }

    if (rl2_get_raster_pixel (NULL, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (NULL, valid)\n");
	  return -221;
      }

    if (rl2_set_raster_pixel (NULL, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (NULL, valid)\n");
	  return -222;
      }

    if (rl2_get_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (mismatching sample)\n");
	  return -223;
      }

    if (rl2_set_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (mismatching sample)\n");
	  return -224;
      }
    rl2_destroy_pixel (pixel);

    pixel = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create UINT8 pixels for testing\n");
	  return -225;
      }

    if (rl2_get_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (mismatching pixel)\n");
	  return -226;
      }

    if (rl2_set_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (mismatching pixel)\n");
	  return -227;
      }
    rl2_destroy_pixel (pixel);

    pixel = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create Gray/256 pixels for testing\n");
	  return -228;
      }

    if (rl2_get_raster_pixel (raster, pixel, 2000, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (invalid row)\n");
	  return -229;
      }

    if (rl2_set_raster_pixel (raster, pixel, 2000, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (invalid row)\n");
	  return -230;
      }

    if (rl2_get_raster_pixel (raster, pixel, 10, 2000) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (invalid col)\n");
	  return -231;
      }

    if (rl2_set_raster_pixel (raster, pixel, 10, 2000) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (invalid col)\n");
	  return -232;
      }
    rl2_destroy_pixel (pixel);

    rl2_destroy_raster (raster);

    return 0;
}
