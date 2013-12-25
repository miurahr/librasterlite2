/*

 test9.c -- RasterLite2 Test Case

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
antiEndian ()
{
/* ensures to always encode in the opposite endian order */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 1;
    return 0;
}

static rl2SectionPtr
create_16gray ()
{
/* creating a synthetic "16gray" image */
    rl2SectionPtr scn;
    rl2RasterPtr rst;
    int row;
    int col;
    int bufsize = 1024 * 1024;
    int idx = 0;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    for (row = 0; row < 256; row += 4)
      {
	  int i;
	  for (i = 0; i < 4; i++)
	    {
		int ix = idx;
		for (col = 0; col < 1024; col += 4)
		  {
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      ix++;
		      if (ix > 15)
			  ix = 0;
		  }
	    }
	  idx++;
	  if (idx > 15)
	      idx = 0;
      }

    for (row = 256; row < 512; row += 8)
      {
	  int i;
	  for (i = 0; i < 8; i++)
	    {
		int ix = idx;
		for (col = 0; col < 1024; col += 8)
		  {
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      ix++;
		      if (ix > 15)
			  ix = 0;
		  }
	    }
	  idx++;
	  if (idx > 15)
	      idx = 0;
      }

    for (row = 512; row < 768; row++)
      {
	  int ix = idx;
	  for (col = 0; col < 1024; col += 8)
	    {
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		ix++;
		if (ix > 15)
		    ix = 0;
	    }
      }

    for (row = 768; row < 1024; row += 8)
      {
	  int i;
	  for (i = 0; i < 8; i++)
	    {
		for (col = 0; col < 1024; col++)
		    *p++ = idx;
	    }
	  idx++;
	  if (idx > 15)
	      idx = 0;
      }

    rst =
	rl2_create_raster (1024, 1024, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, bufsize, NULL, NULL, 0, NULL);
    if (rst == NULL)
	goto error;

    scn = rl2_create_section ("16gray", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      rst);
    if (scn == NULL)
	rl2_destroy_raster (rst);
    return scn;
  error:
    free (bufpix);
    return NULL;
}

static int
test_16grays (rl2SectionPtr img)
{
/* testing 16-GRAYS buffer functions */
    unsigned char *buffer;
    int buf_size;
    int width = 1024;
    unsigned char *p_data1;
    unsigned char *p_data2;
    unsigned char sample;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    rl2PixelPtr pxl;
    rl2RasterPtr rst;

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "16-GRAYS invalid raster pointer\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB data: 16-GRAYS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (720 * width * 3) + (510 * 3);
    if (*(p_data1 + 0) != 171 || *(p_data1 + 1) != 171 || *(p_data1 + 2) != 171)
      {
	  fprintf (stderr, "Unexpected RGB pixel #1: 16-GRAYS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 255 || *(p_data2 + 2) != 255)
      {
	  fprintf (stderr, "Unexpected RGB pixel #2: 16-GRAYS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: 16-GRAYS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 171 || *(p_data1 + 1) != 171 || *(p_data1 + 2) != 171
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #1: 16-GRAYS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 255 || *(p_data2 + 2) != 255
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #2: 16-GRAYS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_ARGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ARGB data: 16-GRAYS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 171 || *(p_data1 + 2) != 171
	|| *(p_data1 + 3) != 171)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #1: 16-GRAYS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 255 || *(p_data2 + 2) != 255
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #2: 16-GRAYS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGR (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGR data: 16-GRAYS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (720 * width * 3) + (510 * 3);
    if (*(p_data1 + 0) != 171 || *(p_data1 + 1) != 171 || *(p_data1 + 2) != 171)
      {
	  fprintf (stderr, "Unexpected BGR pixel #1: 16-GRAYS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 255 || *(p_data2 + 2) != 255)
      {
	  fprintf (stderr, "Unexpected BGR pixel #2: 16-GRAYS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGRA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGRA data: 16-GRAYS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 171 || *(p_data1 + 1) != 171 || *(p_data1 + 2) != 171
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #1: 16-GRAYS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 255 || *(p_data2 + 2) != 255
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #2: 16-GRAYS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_4bit (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get 4-BIT data: 16-GRAYS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (720 * width) + 510;
    if (*p_data1 != 10)
      {
	  fprintf (stderr, "Unexpected 4-BIT pixel #1: 16-GRAYS\n");
	  return 0;
      }
    if (*p_data2 != 15)
      {
	  fprintf (stderr, "Unexpected 4-BIT pixel #2: 16-GRAYS\n");
	  return 0;
      }
    rl2_free (buffer);

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr, "Unable to create Pixel for Raster: 16-GRAYS\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: 16-GRAYS\n");
	  return 0;
      }

    if (rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to create get Pixel Type: 16-GRAYS\n");
	  return 0;
      }

    if (sample_type != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr, "Unexpected Pixel SampleType: 16-GRAYS\n");
	  return 0;
      }

    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected Pixel Type: 16-GRAYS\n");
	  return 0;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected Pixel # Bands: 16-GRAYS\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_4bit (pxl, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel 4-BIT: 16-GRAYS\n");
	  return 0;
      }

    if (sample != 10)
      {
	  fprintf (stderr,
		   "Unexpected Pixel 4-BIT Sample value %d: 16-GRAYS\n",
		   sample);
	  return 0;
      }

    if (rl2_set_pixel_sample_4bit (pxl, sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel SetSample 4-BIT: 16-GRAYS\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_4bit (pxl, 16) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample 4-BIT (exceeding value): 16-GRAYS\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: 16-GRAYS\n");
	  return 0;
      }

    rl2_destroy_pixel (pxl);

    return 1;
}

int
main (int argc, char *argv[])
{
    rl2SectionPtr img;
    rl2RasterPtr raster;
    rl2RasterStatisticsPtr stats;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    unsigned char *blob_odd_png;
    int blob_odd_sz_png;
    unsigned char *blob_even_png;
    int blob_even_sz_png;
    unsigned char *blob_odd_gif;
    int blob_odd_sz_gif;
    unsigned char *blob_even_gif;
    int blob_even_sz_gif;
    unsigned char *blob_stat;
    int blob_stat_size;
    int anti_endian = antiEndian ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    img = create_16gray ();
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create image: 16gray\n");
	  return -1;
      }

    if (rl2_section_to_jpeg (img, "./16gray.jpg", 70) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 16gray.jpg\n");
	  return -2;
      }

    if (rl2_section_to_png (img, "./16gray.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 16gray.png\n");
	  return -3;
      }

    if (!test_16grays (img))
	return -4;

    rl2_destroy_section (img);

    unlink ("./16gray.jpg");

    img = rl2_section_from_png ("./16gray.png");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./16gray.png");
	  return -5;
      }

    unlink ("./16gray.png");

    if (rl2_section_to_png (img, "./from_16gray.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_16gray.png\n");
	  return -6;
      }

    unlink ("./from_16gray.png");

    raster = rl2_get_section_raster (img);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to retrieve the raster pointer\n");
	  return -7;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_NONE, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - uncompressed\n");
	  return -8;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_PNG, &blob_odd_png, &blob_odd_sz_png,
	 &blob_even_png, &blob_even_sz_png, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - PNG\n");
	  return -9;
      }

    stats =
	rl2_get_raster_statistics (blob_odd_png, blob_odd_sz_png, blob_even_png,
				   blob_even_sz_png, NULL, NULL);
    if (stats == NULL)
      {
	  fprintf (stderr, "Unable to get Raster Statistics\n");
	  return -100;
      }
    if (rl2_serialize_dbms_raster_statistics
	(stats, &blob_stat, &blob_stat_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to serialize Raster Statistics\n");
	  return -101;
      }
    rl2_destroy_raster_statistics (stats);
    stats = rl2_deserialize_dbms_raster_statistics (blob_stat, blob_stat_size);
    if (stats == NULL)
      {
	  fprintf (stderr, "Unable to deserialize Raster Statistics\n");
	  return -102;
      }
    free (blob_stat);
    rl2_destroy_raster_statistics (stats);

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_GIF, &blob_odd_gif, &blob_odd_sz_gif,
	 &blob_even_gif, &blob_even_sz_gif, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - GIF\n");
	  return -10;
      }

    rl2_destroy_section (img);

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - uncompressed\n");
	  return -11;
      }

    img = rl2_create_section ("16gray 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - uncompressed\n");
	  return -12;
      }

    if (rl2_section_to_png (img, "./16gray_1_1_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 16gray_1_1_uncompressed.png\n");
	  return -13;
      }
    rl2_destroy_section (img);

    unlink ("./16gray_1_1_uncompressed.png");

    if (rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:2 - uncompressed\n");
	  return -14;
      }

    if (rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:4 - uncompressed\n");
	  return -15;
      }

    if (rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:8 - uncompressed\n");
	  return -16;
      }
    free (blob_odd);

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - png\n");
	  return -17;
      }

    img = rl2_create_section ("16gray 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - png\n");
	  return -18;
      }

    if (rl2_section_to_png (img, "./16gray_1_1_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 16gray_1_1_png.png\n");
	  return -19;
      }
    rl2_destroy_section (img);

    unlink ("./16gray_1_1_png.png");

    if (rl2_raster_decode
	(RL2_SCALE_2, blob_odd_png, blob_odd_sz_png, blob_even_png,
	 blob_even_sz_png, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:2 - png\n");
	  return -20;
      }
    free (blob_odd_png);

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_gif, blob_odd_sz_gif,
			   blob_even_gif, blob_even_sz_gif, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - gif\n");
	  return -21;
      }

    img = rl2_create_section ("16gray 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - gif\n");
	  return -22;
      }

    if (rl2_section_to_png (img, "./16gray_1_1_gif.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 16gray_1_1_gif.png\n");
	  return -23;
      }
    rl2_destroy_section (img);

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_gif, blob_odd_sz_gif,
			   blob_even_gif, blob_even_sz_gif, NULL);
    if (raster != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:2 - gif\n");
	  return -24;
      }
    free (blob_odd_gif);

    unlink ("./16gray_1_1_gif.png");

    return 0;
}
