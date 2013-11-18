/*

 test_wr_tiff.c -- RasterLite2 Test Case

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
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"

static unsigned char *
create_rainbow ()
{
/* creating a synthetic "rainbow" RGB buffer */
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    red = 0;
    for (row = 0; row < 256; row++)
      {
	  green = 0;
	  blue = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		green++;
		blue--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = red;
		*p++ = 127;
		*p++ = 127;
	    }
	  green = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = 255 - red;
		*p++ = green;
		*p++ = blue;
		green--;
		blue++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = 127;
		*p++ = red;
		*p++ = red;
	    }
	  red++;
      }

    green = 0;
    for (row = 256; row < 512; row++)
      {
	  red = 0;
	  blue = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		red++;
		blue--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = 127;
		*p++ = green;
		*p++ = 127;
	    }
	  red = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = red;
		*p++ = 255 - green;
		*p++ = blue;
		red--;
		blue++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = green;
		*p++ = 127;
		*p++ = green;
	    }
	  green++;
      }

    blue = 0;
    for (row = 512; row < 768; row++)
      {
	  red = 0;
	  green = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		red++;
		green--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = 127;
		*p++ = 127;
		*p++ = blue;
	    }
	  red = 255;
	  green = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = 255 - blue;
		red--;
		green++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = blue;
		*p++ = blue;
		*p++ = 127;
	    }
	  blue++;
      }

    red = 0;
    for (row = 768; row < 1024; row++)
      {
	  for (col = 0; col < 256; col++)
	    {
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = red;
		*p++ = red;
		*p++ = red;
	    }
	  green = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = 255 - red;
		*p++ = 255 - red;
		*p++ = 255 - red;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = 255;
		*p++ = 255;
		*p++ = 255;
	    }
	  red++;
      }
    return bufpix;
}

static unsigned char *
create_grayband ()
{
/* creating a synthetic "grayband" RGB buffer */
    int row;
    int col;
    unsigned char gray;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    gray = 0;
    for (row = 0; row < 256; row++)
      {
	  for (col = 0; col < 1024; col++) {
	      *p++ = gray;
	      *p++ = gray;
	      *p++ = gray;
          }
	  gray++;
      }

    gray = 255;
    for (row = 256; row < 512; row++)
      {
	  for (col = 0; col < 1024; col++) {
	      *p++ = gray;
	      *p++ = gray;
	      *p++ = gray;
	  }
          gray--;
      }

    gray = 0;
    for (row = 512; row < 768; row++)
      {
	  for (col = 0; col < 1024; col++) {
	      *p++ = gray;
	      *p++ = gray;
	      *p++ = gray;
          }
	  gray++;
      }

    gray = 255;
    for (row = 768; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++) {
	      *p++ = gray;
	      *p++ = gray;
	      *p++ = gray;
          }
	  gray--;
      }
    return bufpix;
}

static unsigned char *
create_palette ()
{
/* creating a synthetic "palette" RGB buffer */
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    red = 0;
    for (row = 0; row < 256; row++)
      {
          green = 0;
          blue = 255;
	  for (col = 0; col < 1024; col++) {
	      *p++ = red;
	      *p++ = green;
	      *p++ = blue;
          }
          if (red > 247)
              red = 0;
          else
	     red += 8;
      }

    red = 255;
    for (row = 256; row < 512; row++)
      {
          green = 255;
          blue = 192;
	  for (col = 0; col < 1024; col++) {
	      *p++ = red;
	      *p++ = green;
	      *p++ = blue;
	  }
          if (red < 9)
              red = 255;
          else
              red -= 8;
      }

    red = 0;
    for (row = 512; row < 768; row++)
      {
          green = 0;
          blue = 128;
	  for (col = 0; col < 1024; col++) {
	      *p++ = red;
	      *p++ = green;
	      *p++ = blue;
          }
          if (red > 247)
              red = 0;
          else
	     red += 8;
      }

    red = 255;
    for (row = 768; row < 1024; row++)
      {
          green = 128;
          blue = 0;
	  for (col = 0; col < 1024; col++) {
	      *p++ = red;
	      *p++ = green;
	      *p++ = blue;
          }
          if (red < 9)
              red = 255;
          else
              red -= 8;
      }
    return bufpix;
}

static unsigned char *
create_monochrome ()
{
/* creating a synthetic "monochrome" RGB buffer */
    int row;
    int col;
    int mode = 0;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    for (row = 0; row < 1024; row++) {
	  for (col = 0; col < 1024; col++) {
	      *p++ = 0;
              *p++ = 0;
              *p++ = 0;
          }
    }
    p = bufpix;

    for (row = 0; row < 256; row += 4) {
	 int mode2 = mode;
	 for (col = 0; col < 1024; col += 4) {
	      if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
	      } else {
		  p += 12;
		  mode2 = 1;
              }
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 4) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		   mode2 = 0;
	        } else {
		   p += 12;
		   mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 4) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 4;
		  mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 4) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 4;
		  mode2 = 1;
		}
	  }
	  if (mode)
	      mode = 0;
	  else
	      mode = 1;
    }

    for (row = 256; row < 512; row += 8) {
	 int mode2 = mode;
	 for (col = 0; col < 1024; col += 8) {
	      if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 24;
		  mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 8;
		  mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 24;
		  mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 8;
		  mode2 = 1;
		}
	  }
	  for (col = 0; col < 1024; col += 8) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 8;
		  mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 24;
		  mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 24;
		  mode2 = 1;
		}
	  }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8) {
	       if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 24;
		  mode2 = 1;
		}
	  }
	  if (mode)
	      mode = 0;
	  else
	      mode = 1;
    }

    for (row = 512; row < 768; row++) {
	 int mode2 = 1;
	 for (col = 0; col < 1024; col += 8) {
              if (mode2) {
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  *p++ = 255;
		  mode2 = 0;
		} else {
		  p += 8;
		  mode2 = 1;
		}
	  }
    }

    mode = 1;
    for (row = 768; row < 1024; row += 8) {
	 int x;
	 for (x = 0; x < 8; x++) {
	      for (col = 0; col < 1024; col++) {
		   if (mode) {
		       *p++ = 255;
		       *p++ = 255;
		       *p++ = 255;
                   } else 
  		   p += 3;
	      }
	  }
	  if (mode)
	      mode = 0;
	  else
	      mode = 1;
    }

    return bufpix;
}

static int
do_one_rgb_test(const unsigned char *rgb, const char *path, int tiled, unsigned char compression)
{
/* performing a single RGB test */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *xpath;
    unsigned short xwidth;
    unsigned short xheight;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
        tile_size = 1;
    tiff = rl2_create_tiff_destination(path, 1024, 1024, RL2_SAMPLE_UINT8, 
         RL2_PIXEL_RGB, 3, NULL, compression, tiled, tile_size);
    if (tiff == NULL) {
       fprintf(stderr, "Unable to create TIFF \"%s\"\n", path);     
       goto error;
    }

    xpath = rl2_get_tiff_destination_path(tiff);
    if (xpath == NULL) {
        fprintf(stderr, "Get destination path: unexpected NULL\n");
        goto error;
    }
    if (strcmp(xpath, path) != 0) {
        fprintf(stderr, "Mismatching destination path \"%s\"\n", xpath);
        goto error;
    }
    if (rl2_get_tiff_destination_size(tiff, &xwidth, &xheight) != RL2_OK) {
        fprintf(stderr, "Get destination size error\n");
        goto error;
    }
    if (xwidth != 1024) {
        fprintf(stderr, "Unexpected Width %d\n", xwidth);
        goto error;
    }
    if (xheight != 1024) {
        fprintf(stderr, "Unexpected Height %d\n", xheight);
        goto error;
    }
    if (rl2_get_tiff_destination_type (tiff, &xsample_type, &xpixel_type, &xnum_bands) != RL2_OK) {
        fprintf(stderr, "Get destination type\n");
        goto error;
    }
    if (xsample_type != RL2_SAMPLE_UINT8) {
        fprintf(stderr, "Unexpected Sample Type %02x\n", xsample_type);
        goto error;
    }
    if (xpixel_type != RL2_PIXEL_RGB) {
        fprintf(stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
        goto error;
    }
    if (xnum_bands != 3) {
        fprintf(stderr, "Unexpected num_bands %d\n", xnum_bands);
        goto error;
    }

    if (tiled) {
    /* Tiled TIFF */
        for (row = 0; row < 1024; row += tile_size) {
            for (col = 0; col < 1024; col += tile_size) {
            /* inserting a TIFF tile */
                bufpix_size = tile_size * tile_size * 3;
                bufpix = (unsigned char *) malloc(bufpix_size);
                p_out = bufpix;
                for (y = 0; y < tile_size; y++) {
                    if (row + y >= 1024) {
                        for (x = 0; x < tile_size; x++) {
                            *p_out++ = 0;
                            *p_out++ = 0;
                            *p_out++ = 0;
                        }
                    }
                    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
                    for (x = 0; x < tile_size; x++) {
                         if (col + x >= 1024) {
                             *p_out++ = 0;
                             *p_out++ = 0;
                             *p_out++ = 0;
                             continue;
                         }
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                    }
                }
                raster = rl2_create_raster(tile_size, tile_size,
                    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3, bufpix, bufpix_size, 
                    NULL, NULL, 0, NULL);
                if (raster == NULL) {
                    fprintf(stderr, "Unable to encode a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                if (rl2_write_tiff_tile(tiff, raster, row, col) != RL2_OK) {
                    rl2_destroy_raster(raster);
                    fprintf(stderr, "Unable to write a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                rl2_destroy_raster(raster);
            }
        }
    } 
    else {
    /* Striped TIFF */
        for (row = 0; row < 1024; row++) {
        /* inserting a TIFF strip */
            bufpix_size = 1024 * 3;
            bufpix = (unsigned char *) malloc(bufpix_size);
            p_in = rgb + (row * 1024 * 3);
            p_out = bufpix;
            for (x = 0; x < 1024; x++) {
            /* feeding the scanline buffer */
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
            }
            raster = rl2_create_raster(1024, 1,
                RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,bufpix, bufpix_size,
                NULL, NULL, 0, NULL);
            if (raster == NULL) {
                fprintf(stderr, "Unable to encode a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            if (rl2_write_tiff_scanline(tiff, raster, row) != RL2_OK) {
                rl2_destroy_raster(raster);
                fprintf(stderr, "Unable to write a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            rl2_destroy_raster(raster);
        }
    }
/* destroying the TIFF destination */
    rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return 0;

error:
    if (tiff != NULL)
        rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return -1;
}

static int
do_one_rgb_test_geotiff1(const unsigned char *rgb, sqlite3 *handle, const char *path, int tiled, unsigned char compression)
{
/* performing a single RGB test - GeoTIFF (1) */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    double res = 180.0 / 1024.0;
    int xsrid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double hres;
    double vres;
    if (tiled == 0)
        tile_size = 1;

    tiff = rl2_create_geotiff_destination(path, handle, 1024, 1024, RL2_SAMPLE_UINT8, 
         RL2_PIXEL_RGB, 3, NULL, compression, tiled, tile_size, 4326, 0.0, -90.0,
         180.0, 90.0, res, res, 0);
    if (tiff == NULL) {
       fprintf(stderr, "Unable to create GeoTIFF \"%s\"\n", path);     
       goto error;
    }

    if (rl2_get_tiff_destination_srid (tiff, &xsrid) != RL2_OK) {
        fprintf(stderr, "Unable to retrieve the destination SRID\n");
        goto error;
    }
    if (xsrid != 4326) {
        fprintf(stderr, "Unexpected destination SRID %d\n", xsrid);
        goto error;
    }
    if (rl2_get_tiff_destination_extent (tiff, &minx, &miny, &maxx, &maxy) != RL2_OK) {
        fprintf(stderr, "Unable to retrieve the destination Extent\n");
        goto error;
    }
    if (minx != 0.0) {
        fprintf(stderr, "Unexpected Extent MinX %1.8f\n", minx);
        goto error;
    }
    if (miny != -90.0) {
        fprintf(stderr, "Unexpected Extent MinY %1.8f\n", miny);
        goto error;
    }
    if (maxx != 180.0) {
        fprintf(stderr, "Unexpected Extent MaxX %1.8f\n", maxx);
        goto error;
    }
    if (maxy != 90.0) {
        fprintf(stderr, "Unexpected Extent MaxY %1.8f\n", maxy);
        goto error;
    }
    if (rl2_get_tiff_destination_resolution (tiff, &hres, &vres) != RL2_OK) {
        fprintf(stderr, "Unable to retrieve the destination Resolution\n");
        goto error;
    }
    if (hres >= 0.17578126 || hres < 0.17578125) {
        fprintf(stderr, "Unexpected Horizontal Resolution %1.8f\n", hres);
        goto error;
    }
    if (vres >= 0.17578126 || vres < 0.17578125) {
        fprintf(stderr, "Unexpected Vertical Resolution %1.8f\n", vres);
        goto error;
    }

    if (tiled) {
    /* Tiled TIFF */
        for (row = 0; row < 1024; row += tile_size) {
            for (col = 0; col < 1024; col += tile_size) {
            /* inserting a TIFF tile */
                bufpix_size = tile_size * tile_size * 3;
                bufpix = (unsigned char *) malloc(bufpix_size);
                p_out = bufpix;
                for (y = 0; y < tile_size; y++) {
                    if (row + y >= 1024) {
                        for (x = 0; x < tile_size; x++) {
                            *p_out++ = 0;
                            *p_out++ = 0;
                            *p_out++ = 0;
                        }
                    }
                    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
                    for (x = 0; x < tile_size; x++) {
                         if (col + x >= 1024) {
                             *p_out++ = 0;
                             *p_out++ = 0;
                             *p_out++ = 0;
                             continue;
                         }
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                    }
                }
                raster = rl2_create_raster(tile_size, tile_size,
                    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3, bufpix, bufpix_size, 
                    NULL, NULL, 0, NULL);
                if (raster == NULL) {
                    fprintf(stderr, "Unable to encode a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                if (rl2_write_tiff_tile(tiff, raster, row, col) != RL2_OK) {
                    rl2_destroy_raster(raster);
                    fprintf(stderr, "Unable to write a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                rl2_destroy_raster(raster);
            }
        }
    } 
    else {
    /* Striped TIFF */
        for (row = 0; row < 1024; row++) {
        /* inserting a TIFF strip */
            bufpix_size = 1024 * 3;
            bufpix = (unsigned char *) malloc(bufpix_size);
            p_in = rgb + (row * 1024 * 3);
            p_out = bufpix;
            for (x = 0; x < 1024; x++) {
            /* feeding the scanline buffer */
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
            }
            raster = rl2_create_raster(1024, 1,
                RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,bufpix, bufpix_size,
                NULL, NULL, 0, NULL);
            if (raster == NULL) {
                fprintf(stderr, "Unable to encode a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            if (rl2_write_tiff_scanline(tiff, raster, row) != RL2_OK) {
                rl2_destroy_raster(raster);
                fprintf(stderr, "Unable to write a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            rl2_destroy_raster(raster);
        }
    }

/* writing a TFW (error expected) */
    if (rl2_write_tiff_worldfile (tiff) != RL2_ERROR) {
        fprintf(stderr, "Unexpected success TIFF TFW\n");
        goto error;
    }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return 0;

error:
    if (tiff != NULL)
        rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return -1;
}

static int
do_one_rgb_test_geotiff2(const unsigned char *rgb, sqlite3 *handle, const char *path, int tiled, unsigned char compression)
{
/* performing a single RGB test - GeoTIFF (2) */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    int xsrid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double hres;
    double vres;
    if (tiled == 0)
        tile_size = 1;

    tiff = rl2_create_geotiff_destination(path, handle, 1024, 1024, RL2_SAMPLE_UINT8, 
         RL2_PIXEL_RGB, 3, NULL, compression, tiled, tile_size, 32632, 100000.0, 1000000.0,
         101024.0, 1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL) {
       fprintf(stderr, "Unable to create GeoTIFF \"%s\"\n", path);     
       goto error;
    }

    if (rl2_get_tiff_destination_srid (tiff, &xsrid) != RL2_OK) {
        fprintf(stderr, "Unable to retrieve the destination SRID\n");
        goto error;
    }
    if (xsrid != 32632) {
        fprintf(stderr, "Unexpected destination SRID %d\n", xsrid);
        goto error;
    }
    if (rl2_get_tiff_destination_extent (tiff, &minx, &miny, &maxx, &maxy) != RL2_OK) {
        fprintf(stderr, "Unable to retrieve the destination Extent\n");
        goto error;
    }
    if (minx != 100000.0) {
        fprintf(stderr, "Unexpected Extent MinX %1.8f\n", minx);
        goto error;
    }
    if (miny != 1000000.0) {
        fprintf(stderr, "Unexpected Extent MinY %1.8f\n", miny);
        goto error;
    }
    if (maxx != 101024.0) {
        fprintf(stderr, "Unexpected Extent MaxX %1.8f\n", maxx);
        goto error;
    }
    if (maxy != 1001024.0) {
        fprintf(stderr, "Unexpected Extent MaxY %1.8f\n", maxy);
        goto error;
    }
    if (rl2_get_tiff_destination_resolution (tiff, &hres, &vres) != RL2_OK) {
        fprintf(stderr, "Unable to retrieve the destination Resolution\n");
        goto error;
    }
    if (hres >= 1.00000001 || hres < 1.00000000) {
        fprintf(stderr, "Unexpected Horizontal Resolution %1.8f\n", hres);
        goto error;
    }
    if (vres >= 1.00000001 || vres < 1.00000000) {
        fprintf(stderr, "Unexpected Vertical Resolution %1.8f\n", vres);
        goto error;
    }

    if (tiled) {
    /* Tiled TIFF */
        for (row = 0; row < 1024; row += tile_size) {
            for (col = 0; col < 1024; col += tile_size) {
            /* inserting a TIFF tile */
                bufpix_size = tile_size * tile_size * 3;
                bufpix = (unsigned char *) malloc(bufpix_size);
                p_out = bufpix;
                for (y = 0; y < tile_size; y++) {
                    if (row + y >= 1024) {
                        for (x = 0; x < tile_size; x++) {
                            *p_out++ = 0;
                            *p_out++ = 0;
                            *p_out++ = 0;
                        }
                    }
                    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
                    for (x = 0; x < tile_size; x++) {
                         if (col + x >= 1024) {
                             *p_out++ = 0;
                             *p_out++ = 0;
                             *p_out++ = 0;
                             continue;
                         }
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                    }
                }
                raster = rl2_create_raster(tile_size, tile_size,
                    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3, bufpix, bufpix_size, 
                    NULL, NULL, 0, NULL);
                if (raster == NULL) {
                    fprintf(stderr, "Unable to encode a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                if (rl2_write_tiff_tile(tiff, raster, row, col) != RL2_OK) {
                    rl2_destroy_raster(raster);
                    fprintf(stderr, "Unable to write a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                rl2_destroy_raster(raster);
            }
        }
    } 
    else {
    /* Striped TIFF */
        for (row = 0; row < 1024; row++) {
        /* inserting a TIFF strip */
            bufpix_size = 1024 * 3;
            bufpix = (unsigned char *) malloc(bufpix_size);
            p_in = rgb + (row * 1024 * 3);
            p_out = bufpix;
            for (x = 0; x < 1024; x++) {
            /* feeding the scanline buffer */
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
            }
            raster = rl2_create_raster(1024, 1,
                RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,bufpix, bufpix_size,
                NULL, NULL, 0, NULL);
            if (raster == NULL) {
                fprintf(stderr, "Unable to encode a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            if (rl2_write_tiff_scanline(tiff, raster, row) != RL2_OK) {
                rl2_destroy_raster(raster);
                fprintf(stderr, "Unable to write a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            rl2_destroy_raster(raster);
        }
    }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK) {
        fprintf(stderr, "Unable to write a TFW\n");
        goto error;
    }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return 0;

error:
    if (tiff != NULL)
        rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return -1;
}

static int
do_one_rgb_test_tfw(const unsigned char *rgb, const char *path, int tiled, unsigned char compression)
{
/* performing a single RGB test - TFW */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    double res = 180.0 / 1024.0;
    if (tiled == 0)
        tile_size = 1;
    tiff = rl2_create_tiff_worldfile_destination(path, 1024, 1024, RL2_SAMPLE_UINT8, 
         RL2_PIXEL_RGB, 3, NULL, compression, tiled, tile_size, 4326, 0.0, -90.0,
         180.0, 90.0, res, res);
    if (tiff == NULL) {
       fprintf(stderr, "Unable to create TIFF+TFW \"%s\"\n", path);     
       goto error;
    }

    if (tiled) {
    /* Tiled TIFF */
        for (row = 0; row < 1024; row += tile_size) {
            for (col = 0; col < 1024; col += tile_size) {
            /* inserting a TIFF tile */
                bufpix_size = tile_size * tile_size * 3;
                bufpix = (unsigned char *) malloc(bufpix_size);
                p_out = bufpix;
                for (y = 0; y < tile_size; y++) {
                    if (row + y >= 1024) {
                        for (x = 0; x < tile_size; x++) {
                            *p_out++ = 0;
                            *p_out++ = 0;
                            *p_out++ = 0;
                        }
                    }
                    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
                    for (x = 0; x < tile_size; x++) {
                         if (col + x >= 1024) {
                             *p_out++ = 0;
                             *p_out++ = 0;
                             *p_out++ = 0;
                             continue;
                         }
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                         *p_out++ = *p_in++;
                    }
                }
                raster = rl2_create_raster(tile_size, tile_size,
                    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3, bufpix, bufpix_size, 
                    NULL, NULL, 0, NULL);
                if (raster == NULL) {
                    fprintf(stderr, "Unable to encode a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                if (rl2_write_tiff_tile(tiff, raster, row, col) != RL2_OK) {
                    rl2_destroy_raster(raster);
                    fprintf(stderr, "Unable to write a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                rl2_destroy_raster(raster);
            }
        }
    } 
    else {
    /* Striped TIFF */
        for (row = 0; row < 1024; row++) {
        /* inserting a TIFF strip */
            bufpix_size = 1024 * 3;
            bufpix = (unsigned char *) malloc(bufpix_size);
            p_in = rgb + (row * 1024 * 3);
            p_out = bufpix;
            for (x = 0; x < 1024; x++) {
            /* feeding the scanline buffer */
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
                  *p_out++ = *p_in++;
            }
            raster = rl2_create_raster(1024, 1,
                RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,bufpix, bufpix_size,
                NULL, NULL, 0, NULL);
            if (raster == NULL) {
                fprintf(stderr, "Unable to encode a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            if (rl2_write_tiff_scanline(tiff, raster, row) != RL2_OK) {
                rl2_destroy_raster(raster);
                fprintf(stderr, "Unable to write a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            rl2_destroy_raster(raster);
        }
    }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK) {
        fprintf(stderr, "Unable to write a TFW\n");
        goto error;
    }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return 0;

error:
    if (tiff != NULL)
        rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return -1;
}

static int
do_one_gray_test(const unsigned char *rgb, const char *path, int tiled, unsigned char compression)
{
/* performing a single GrayBand test */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
        tile_size = 1;
    tiff = rl2_create_tiff_destination(path, 1024, 1024, RL2_SAMPLE_UINT8, 
         RL2_PIXEL_GRAYSCALE, 1, NULL, compression, tiled, tile_size);
    if (tiff == NULL) {
       fprintf(stderr, "Unable to create TIFF \"%s\"\n", path);     
       goto error;
    }

    if (rl2_get_tiff_destination_type (tiff, &xsample_type, &xpixel_type, &xnum_bands) != RL2_OK) {
        fprintf(stderr, "Get destination type\n");
        goto error;
    }
    if (xsample_type != RL2_SAMPLE_UINT8) {
        fprintf(stderr, "Unexpected Sample Type %02x\n", xsample_type);
        goto error;
    }
    if (xpixel_type != RL2_PIXEL_GRAYSCALE) {
        fprintf(stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
        goto error;
    }
    if (xnum_bands != 1) {
        fprintf(stderr, "Unexpected num_bands %d\n", xnum_bands);
        goto error;
    }

    if (tiled) {
    /* Tiled TIFF */
        for (row = 0; row < 1024; row += tile_size) {
            for (col = 0; col < 1024; col += tile_size) {
            /* inserting a TIFF tile */
                bufpix_size = tile_size * tile_size;
                bufpix = (unsigned char *) malloc(bufpix_size);
                p_out = bufpix;
                for (y = 0; y < tile_size; y++) {
                    if (row + y >= 1024) {
                        for (x = 0; x < tile_size; x++)
                            *p_out++ = 0;
                    }
                    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
                    for (x = 0; x < tile_size; x++) {
                         if (col + x >= 1024) {
                             *p_out++ = 0;
                             p_in += 2;
                             continue;
                         }
                         *p_out++ = *p_in++;
                         p_in += 2;
                    }
                }
                raster = rl2_create_raster(tile_size, tile_size,
                    RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1, bufpix, bufpix_size, 
                    NULL, NULL, 0, NULL);
                if (raster == NULL) {
                    fprintf(stderr, "Unable to encode a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                if (rl2_write_tiff_tile(tiff, raster, row, col) != RL2_OK) {
                    rl2_destroy_raster(raster);
                    fprintf(stderr, "Unable to write a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                rl2_destroy_raster(raster);
            }
        }
    } 
    else {
    /* Striped TIFF */
        for (row = 0; row < 1024; row++) {
        /* inserting a TIFF strip */
            bufpix_size = 1024;
            bufpix = (unsigned char *) malloc(bufpix_size);
            p_in = rgb + (row * 1024 * 3);
            p_out = bufpix;
            for (x = 0; x < 1024; x++) {
            /* feeding the scanline buffer */
                  *p_out++ = *p_in++;
                  p_in += 2;
            }
            raster = rl2_create_raster(1024, 1,
                RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1, bufpix, bufpix_size,
                NULL, NULL, 0, NULL);
            if (raster == NULL) {
                fprintf(stderr, "Unable to encode a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            if (rl2_write_tiff_scanline(tiff, raster, row) != RL2_OK) {
                rl2_destroy_raster(raster);
                fprintf(stderr, "Unable to write a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            rl2_destroy_raster(raster);
        }
    }
/* destroying the TIFF destination */
    rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return 0;

error:
    if (tiff != NULL)
        rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return -1;
}

static void
add_palette_color(int *r, int *g, int *b, unsigned char red, unsigned char green,
    unsigned char blue)
{
/* inserting a new color into the Palette */
    int i;
    for (i = 0; i < 255; i++) {
         if (*(r+i) == red && *(g+i) == green && *(b+i) == blue)
             return;
    }
    for (i = 0; i < 255; i++) {
         if (*(r+i) == -1 && *(g+i) == -1 && *(b+i) == -1) {
             *(r+i) = red;
             *(g+i) = green;
             *(b+i) = blue;
             return;
         }
    }
}

static rl2PalettePtr
build_palette(const unsigned char *rgb, int *r, int *g, int *b)
{
/* building a Palette matching the RGB pix-buffer */
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    const unsigned char *p_in = rgb;
    rl2PalettePtr palette;
    if (rgb != NULL) {
        for (row = 0; row < 1024; row++) {
             for (col = 0; col < 1024; col++) {
                 red = *p_in++;
                 green = *p_in++;
                 blue = *p_in++;
                 add_palette_color(r, g, b, red, green, blue);
             }
         }
    }
    palette = rl2_create_palette(256);
    for (row = 0; row < 256; row++)
         rl2_set_palette_color(palette, row, *(r+row), *(g+row), *(b+row), 255);
    return palette;
}

static int
do_one_palette_test(const unsigned char *rgb, const char *path, int tiled, unsigned char compression)
{
/* performing a single Palette test */
    int row;
    int col;
    int x;
    int y;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char index;
    int r[256];
    int g[256];
    int b[256];
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    rl2PalettePtr palette;
    rl2PalettePtr palette2;
    int tile_size = 128;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
        tile_size = 1;

    for (row = 0; row < 256; row++) {
         r[row] = -1;
         g[row] = -1;
         b[row] = -1;
    }
    palette = build_palette(rgb, r, g, b);
    tiff = rl2_create_tiff_destination(path, 1024, 1024, RL2_SAMPLE_UINT8, 
         RL2_PIXEL_PALETTE, 1, palette, compression, tiled, tile_size);
    if (tiff == NULL) {
       fprintf(stderr, "Unable to create TIFF \"%s\"\n", path);     
       goto error;
    }

    if (rl2_get_tiff_destination_type (tiff, &xsample_type, &xpixel_type, &xnum_bands) != RL2_OK) {
        fprintf(stderr, "Get destination type\n");
        goto error;
    }
    if (xsample_type != RL2_SAMPLE_UINT8) {
        fprintf(stderr, "Unexpected Sample Type %02x\n", xsample_type);
        goto error;
    }
    if (xpixel_type != RL2_PIXEL_PALETTE) {
        fprintf(stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
        goto error;
    }
    if (xnum_bands != 1) {
        fprintf(stderr, "Unexpected num_bands %d\n", xnum_bands);
        goto error;
    }

    if (tiled) {
    /* Tiled TIFF */
        for (row = 0; row < 1024; row += tile_size) {
            for (col = 0; col < 1024; col += tile_size) {
            /* inserting a TIFF tile */ 
                palette2 = build_palette(NULL, r, g, b);
                bufpix_size = tile_size * tile_size;
                bufpix = (unsigned char *) malloc(bufpix_size);
                p_out = bufpix;
                for (y = 0; y < tile_size; y++) {
                    if (row + y >= 1024) {
                        for (x = 0; x < tile_size; x++)
                            *p_out++ = 0;
                    }
                    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
                    for (x = 0; x < tile_size; x++) {
                         if (col + x >= 1024) {
                             *p_out++ = 0;
                             p_in += 2;
                             continue;
                         }
                         red = *p_in++;
                         green = *p_in++;
                         blue = *p_in++;
                         rl2_get_palette_index(palette2, &index, red, green, blue, 255);
                         *p_out++ = index;
                    }
                }
                raster = rl2_create_raster(tile_size, tile_size,
                    RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1, bufpix, bufpix_size, 
                    palette2, NULL, 0, NULL);
                if (raster == NULL) {
                    fprintf(stderr, "Unable to encode a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                if (rl2_write_tiff_tile(tiff, raster, row, col) != RL2_OK) {
                    rl2_destroy_raster(raster);
                    fprintf(stderr, "Unable to write a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                rl2_destroy_raster(raster);
            }
        }
    } 
    else {
    /* Striped TIFF */
        for (row = 0; row < 1024; row++) {
        /* inserting a TIFF strip */
            palette2 = build_palette(NULL, r, g, b);
            bufpix_size = 1024;
            bufpix = (unsigned char *) malloc(bufpix_size);
            p_in = rgb + (row * 1024 * 3);
            p_out = bufpix;
            for (x = 0; x < 1024; x++) {
            /* feeding the scanline buffer */
                  red = *p_in++;
                  green = *p_in++;
                  blue = *p_in++;
                  rl2_get_palette_index(palette2, &index, red, green, blue, 255);
                  *p_out++ = index;
            }
            raster = rl2_create_raster(1024, 1,
                RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1, bufpix, bufpix_size,
                palette2, NULL, 0, NULL);
            if (raster == NULL) {
                fprintf(stderr, "Unable to encode a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            if (rl2_write_tiff_scanline(tiff, raster, row) != RL2_OK) {
                rl2_destroy_raster(raster);
                fprintf(stderr, "Unable to write a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            rl2_destroy_raster(raster);
        }
    }
/* destroying the TIFF destination */
    rl2_destroy_palette(palette);
    rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return 0;

error:
    rl2_destroy_palette(palette);
    if (tiff != NULL)
        rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return -1;
}

static int
do_one_monochrome_test(const unsigned char *rgb, const char *path, int tiled, unsigned char compression)
{
/* performing a single Monochrome test */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
        tile_size = 1;
    tiff = rl2_create_tiff_destination(path, 1024, 1024, RL2_SAMPLE_1_BIT, 
         RL2_PIXEL_MONOCHROME, 1, NULL, compression, tiled, tile_size);
    if (tiff == NULL) {
       fprintf(stderr, "Unable to create TIFF \"%s\"\n", path);     
       goto error;
    }

    if (rl2_get_tiff_destination_type (tiff, &xsample_type, &xpixel_type, &xnum_bands) != RL2_OK) {
        fprintf(stderr, "Get destination type\n");
        goto error;
    }
    if (xsample_type != RL2_SAMPLE_1_BIT) {
        fprintf(stderr, "Unexpected Sample Type %02x\n", xsample_type);
        goto error;
    }
    if (xpixel_type != RL2_PIXEL_MONOCHROME) {
        fprintf(stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
        goto error;
    }
    if (xnum_bands != 1) {
        fprintf(stderr, "Unexpected num_bands %d\n", xnum_bands);
        goto error;
    }

    if (tiled) {
    /* Tiled TIFF */
        for (row = 0; row < 1024; row += tile_size) {
            for (col = 0; col < 1024; col += tile_size) {
            /* inserting a TIFF tile */
                bufpix_size = tile_size * tile_size;
                bufpix = (unsigned char *) malloc(bufpix_size);
                p_out = bufpix;
                for (y = 0; y < tile_size; y++) {
                    if (row + y >= 1024) {
                        for (x = 0; x < tile_size; x++)
                            *p_out++ = 0;
                    }
                    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
                    for (x = 0; x < tile_size; x++) {
                         if (col + x >= 1024) {
                             *p_out++ = 0;
                             p_in += 2;
                             continue;
                         }
                         if (*p_in++ < 128)
                             *p_out++ = 1;
                         else
                             *p_out++ = 0;
                         p_in += 2;
                    }
                }
                raster = rl2_create_raster(tile_size, tile_size,
                    RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1, bufpix, bufpix_size, 
                    NULL, NULL, 0, NULL);
                if (raster == NULL) {
                    fprintf(stderr, "Unable to encode a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                if (rl2_write_tiff_tile(tiff, raster, row, col) != RL2_OK) {
                    rl2_destroy_raster(raster);
                    fprintf(stderr, "Unable to write a tile \"%s\" row=%d col=%d\n", path, row, col);
                    goto error;
                }
                rl2_destroy_raster(raster);
            }
        }
    } 
    else {
    /* Striped TIFF */
        for (row = 0; row < 1024; row++) {
        /* inserting a TIFF strip */
            bufpix_size = 1024;
            bufpix = (unsigned char *) malloc(bufpix_size);
            p_in = rgb + (row * 1024 * 3);
            p_out = bufpix;
            for (x = 0; x < 1024; x++) {
            /* feeding the scanline buffer */
                  if (*p_in++ < 128)
                      *p_out++ = 1;
                  else
                      *p_out++ = 0;
                  p_in += 2;
            }
            raster = rl2_create_raster(1024, 1,
                RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1, bufpix, bufpix_size,
                NULL, NULL, 0, NULL);
            if (raster == NULL) {
                fprintf(stderr, "Unable to encode a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            if (rl2_write_tiff_scanline(tiff, raster, row) != RL2_OK) {
                rl2_destroy_raster(raster);
                fprintf(stderr, "Unable to write a scanline \"%s\" row=%d\n", path, row);
                goto error;
            }
            rl2_destroy_raster(raster);
        }
    }
/* destroying the TIFF destination */
    rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return 0;

error:
    if (tiff != NULL)
        rl2_destroy_tiff_destination(tiff);
    unlink (path);
    return -1;
}

static int
do_test_rgb(const unsigned char *rgb)
{
/* testing RGB flavours */
    int ret = do_one_rgb_test(rgb, "./rgb_strip_none.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test(rgb, "./rgb_strip_lzw.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test(rgb, "./rgb_strip_deflate.tif", 0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test(rgb, "./rgb_strip_jpeg.tif", 0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test(rgb, "./rgb_tile_none.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test(rgb, "./rgb_tile_lzw.tif", 01, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test(rgb, "./rgb_tile_deflate.tif", 1, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test(rgb, "./rgb_tile_jpeg.tif", 1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    return 0;
}

static int
do_test_rgb_geotiff1(const unsigned char *rgb, sqlite3 *handle)
{
/* testing RGB flavours - GeoTIFF (1) */
    int ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_strip_none_geotiff1.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_strip_lzw_geotiff1.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_strip_deflate_geotiff1.tif", 0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_strip_jpeg_geotiff1.tif", 0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_tile_none_geotiff1.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_tile_lzw_geotiff1.tif", 01, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_tile_deflate_geotiff1.tif", 1, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff1(rgb, handle, "./rgb_tile_jpeg_geotiff1.tif", 1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    return 0;
}

static int
do_test_rgb_geotiff2(const unsigned char *rgb, sqlite3 *handle)
{
/* testing RGB flavours - GeoTIFF (2) */
    int ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_strip_none_geotiff2.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_strip_lzw_geotiff2.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_strip_deflate_geotiff2.tif", 0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_strip_jpeg_geotiff2.tif", 0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_tile_none_geotiff2.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_tile_lzw_geotiff2.tif", 01, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_tile_deflate_geotiff2.tif", 1, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_geotiff2(rgb, handle, "./rgb_tile_jpeg_geotiff2.tif", 1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    return 0;
}

static int
do_test_rgb_tfw(const unsigned char *rgb)
{
/* testing RGB flavours - TFW */
    int ret = do_one_rgb_test_tfw(rgb, "./rgb_strip_none_tfw.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_tfw(rgb, "./rgb_strip_lzw_tfw.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_tfw(rgb, "./rgb_strip_deflate_tfw.tif", 0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_tfw(rgb, "./rgb_strip_jpeg_tfw.tif", 0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_tfw(rgb, "./rgb_tile_none_tfw.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_tfw(rgb, "./rgb_tile_lzw_tfw.tif", 01, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_tfw(rgb, "./rgb_tile_deflate_tfw.tif", 1, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_rgb_test_tfw(rgb, "./rgb_tile_jpeg_tfw.tif", 1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    return 0;
}

static int
do_test_grayband(const unsigned char *rgb)
{
/* testing GrayBand flavours */
    int ret = do_one_gray_test(rgb, "./gray_strip_none.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_gray_test(rgb, "./gray_strip_lzw.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_gray_test(rgb, "./gray_strip_deflate.tif", 0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_gray_test(rgb, "./gray_strip_jpeg.tif", 0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    ret = do_one_gray_test(rgb, "./gray_tile_none.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_gray_test(rgb, "./gray_tile_lzw.tif", 01, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_gray_test(rgb, "./gray_tile_deflate.tif", 1, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_gray_test(rgb, "./gray_tile_jpeg.tif", 1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
        return ret;
    return 0;
}

static int
do_test_palette(const unsigned char *rgb)
{
/* testing Palette flavours */
    int ret = do_one_palette_test(rgb, "./palette_strip_none.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_palette_test(rgb, "./palette_strip_lzw.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_palette_test(rgb, "./palette_strip_deflate.tif", 0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    ret = do_one_palette_test(rgb, "./palette_tile_none.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_palette_test(rgb, "./palette_tile_lzw.tif", 01, RL2_COMPRESSION_LZW);
    if (ret < 0)
        return ret;
    ret = do_one_palette_test(rgb, "./palette_tile_deflate.tif", 1, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
        return ret;
    return 0;
}

static int
do_test_monochrome(const unsigned char *rgb)
{
/* testing Monochrome flavours */
    int ret = do_one_monochrome_test(rgb, "./monochrome_strip_none.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_monochrome_test(rgb, "./monochrome_strip_fax3.tif", 0, RL2_COMPRESSION_CCITTFAX3);
    if (ret < 0)
        return ret;
    ret = do_one_monochrome_test(rgb, "./monochrome_strip_fax4.tif", 0, RL2_COMPRESSION_CCITTFAX4);
    if (ret < 0)
        return ret;
    ret = do_one_monochrome_test(rgb, "./monochrome_tile_none.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
        return ret;
    ret = do_one_monochrome_test(rgb, "./monochrome_tile_fax3.tif", 01, RL2_COMPRESSION_CCITTFAX3);
    if (ret < 0)
        return ret;
    ret = do_one_monochrome_test(rgb, "./monochrome_tile_fax4.tif", 1, RL2_COMPRESSION_CCITTFAX4);
    if (ret < 0)
        return ret;
    return 0;
}

int
main (int argc, char *argv[])
{
    int ret;
    unsigned char *rgb;
    sqlite3 *handle = NULL;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (handle));
	return -2;
    }
    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -3;
    }

/* testing an RGB image */
    rgb = create_rainbow ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: rainbow\n");
	  return -1;
      }
/* testing RGB flavours */
    ret = do_test_rgb(rgb);    
    if (ret < 0)
        return ret;
/* testing RGB flavours - GeoTIFF (1) */
    ret = do_test_rgb_geotiff1(rgb, handle);  
    if (ret < 0)
        return ret; 
/* testing RGB flavours - GeoTIFF (3) */
    ret = do_test_rgb_geotiff2(rgb, handle); 
    if (ret < 0)
        return ret; 
/* testing RGB flavours - TIFF+TFW */
    ret = do_test_rgb_tfw(rgb); 
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
        return ret;

/* testing GrayBand image */
    rgb = create_grayband ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: grayband\n");
	  return -1;
      }
/* testing GrayBand flavours */
    ret = do_test_grayband(rgb);    
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
        return ret;

/* testing Palette image */
    rgb = create_palette ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: palette\n");
	  return -1;
      }
/* testing Palete flavours */
    ret = do_test_palette(rgb);    
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
        return ret;

/* testing Monochrome image */
    rgb = create_monochrome ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: monochrome\n");
	  return -1;
      }
/* testing Monochrome flavours */
    ret = do_test_monochrome(rgb);    
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
        return ret;
    
    sqlite3_close(handle);
    spatialite_cleanup_ex(cache);

    spatialite_shutdown();
    return 0;
}
