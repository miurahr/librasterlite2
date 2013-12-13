/*

 test_palette.c -- RasterLite2 Test Case

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
#include <stdio.h>

#include "rasterlite2/rasterlite2.h"

rl2PalettePtr
create_monochrome_palette ()
{
/* creating a MONOCHROME palette */
    rl2PalettePtr palette = rl2_create_palette (2);
    rl2_set_palette_color (palette, 0, 255, 255, 255, 255);
    rl2_set_palette_color (palette, 1, 0, 0, 0, 255);
    return palette;
}

rl2PalettePtr
create_gray4_palette ()
{
/* creating a GRAY-4 palette */
    rl2PalettePtr palette = rl2_create_palette (4);
    rl2_set_palette_color (palette, 0, 0, 0, 0, 255);
    rl2_set_palette_color (palette, 1, 86, 86, 86, 255);
    rl2_set_palette_color (palette, 2, 170, 170, 170, 255);
    rl2_set_palette_color (palette, 3, 255, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_gray16_palette ()
{
/* creating a GRAY-16 palette */
    rl2PalettePtr palette = rl2_create_palette (16);
    rl2_set_palette_color (palette, 0, 0, 0, 0, 255);
    rl2_set_palette_color (palette, 1, 17, 17, 17, 255);
    rl2_set_palette_color (palette, 2, 34, 34, 34, 255);
    rl2_set_palette_color (palette, 3, 51, 51, 51, 255);
    rl2_set_palette_color (palette, 4, 68, 68, 68, 255);
    rl2_set_palette_color (palette, 5, 85, 85, 85, 255);
    rl2_set_palette_color (palette, 6, 102, 102, 102, 255);
    rl2_set_palette_color (palette, 7, 119, 119, 119, 255);
    rl2_set_palette_color (palette, 8, 137, 137, 137, 255);
    rl2_set_palette_color (palette, 9, 154, 154, 154, 255);
    rl2_set_palette_color (palette, 10, 171, 171, 171, 255);
    rl2_set_palette_color (palette, 11, 188, 188, 188, 255);
    rl2_set_palette_color (palette, 12, 205, 205, 205, 255);
    rl2_set_palette_color (palette, 13, 222, 222, 222, 255);
    rl2_set_palette_color (palette, 14, 239, 239, 239, 255);
    rl2_set_palette_color (palette, 15, 255, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_gray256_palette ()
{
/* creating a GRAY-256 palette */
    int i;
    rl2PalettePtr palette = rl2_create_palette (256);
    for (i = 0; i < 256; i++)
	rl2_set_palette_color (palette, i, i, i, i, 255);
    return palette;
}

rl2PalettePtr
create_bicolor_palette ()
{
/* creating a BICOLOR palette */
    rl2PalettePtr palette = rl2_create_palette (2);
    rl2_set_palette_color (palette, 0, 255, 0, 255, 255);
    rl2_set_palette_color (palette, 1, 255, 0, 255, 255);
    return palette;
}

rl2PalettePtr
create_rgb4_palette ()
{
/* creating an RGB-4 palette */
    rl2PalettePtr palette = rl2_create_palette (4);
    rl2_set_palette_color (palette, 0, 255, 0, 0, 255);
    rl2_set_palette_color (palette, 1, 255, 86, 86, 255);
    rl2_set_palette_color (palette, 2, 255, 170, 170, 255);
    rl2_set_palette_color (palette, 3, 255, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_rgb16_palette ()
{
/* creating an RGB-16 palette */
    rl2PalettePtr palette = rl2_create_palette (16);
    rl2_set_palette_color (palette, 0, 255, 0, 0, 255);
    rl2_set_palette_color (palette, 1, 255, 17, 17, 255);
    rl2_set_palette_color (palette, 2, 255, 34, 34, 255);
    rl2_set_palette_color (palette, 3, 255, 51, 51, 255);
    rl2_set_palette_color (palette, 4, 68, 255, 68, 255);
    rl2_set_palette_color (palette, 5, 85, 255, 85, 255);
    rl2_set_palette_color (palette, 6, 102, 255, 102, 255);
    rl2_set_palette_color (palette, 7, 119, 255, 119, 255);
    rl2_set_palette_color (palette, 8, 137, 255, 137, 255);
    rl2_set_palette_color (palette, 9, 154, 255, 154, 255);
    rl2_set_palette_color (palette, 10, 171, 171, 255, 255);
    rl2_set_palette_color (palette, 11, 188, 188, 255, 255);
    rl2_set_palette_color (palette, 12, 205, 205, 255, 255);
    rl2_set_palette_color (palette, 13, 222, 222, 255, 255);
    rl2_set_palette_color (palette, 14, 239, 239, 255, 255);
    rl2_set_palette_color (palette, 15, 255, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_rgb256_palette ()
{
/* creating an RGB-256 palette */
    int i;
    rl2PalettePtr palette = rl2_create_palette (256);
    for (i = 0; i < 256; i++)
	rl2_set_palette_color (palette, i, 255 - i, i, 128, 255);
    return palette;
}

int
main (int argc, char *argv[])
{
    rl2PalettePtr palette;
    rl2PalettePtr plt2;
    unsigned short num_entries;
    unsigned char *r;
    unsigned char *g;
    unsigned char *b;
    unsigned char *a;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char *blob;
    int blob_size;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (rl2_create_palette (-1) != NULL)
      {
	  fprintf (stderr, "Unexpected valid palette - negative # entries\n");
	  return -1;
      }

    palette = rl2_create_palette (0);
    if (palette == NULL)
      {
	  fprintf (stderr, "Unexpected invalid palette - ZERO # entries\n");
	  return -2;
      }
    rl2_destroy_palette (palette);

    if (rl2_create_palette (257) != NULL)
      {
	  fprintf (stderr, "Unexpected valid palette - 257 # entries\n");
	  return -3;
      }

    palette = rl2_create_palette (2);
    if (palette == NULL)
      {
	  fprintf (stderr, "Unable to create a valid palette\n");
	  return -4;
      }

    if (rl2_set_palette_color (palette, 0, 0, 0, 0, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set palette color #0\n");
	  return -5;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#aBcDeF") != RL2_OK)
      {
	  fprintf (stderr, "Unable to set palette color #1\n");
	  return -6;
      }

    if (rl2_get_palette_entries (palette, &num_entries) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get palette # entries\n");
	  return -7;
      }

    if (num_entries != 2)
      {
	  fprintf (stderr, "Unexpected palette # entries\n");
	  return -8;
      }

    if (rl2_get_palette_colors (palette, &num_entries, &r, &g, &b, &a) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve palette colors\n");
	  return -9;
      }
    if (num_entries != 2)
      {
	  fprintf (stderr, "Unexpected palette # colors\n");
	  return -10;
      }
    if (*(r + 0) != 0 || *(g + 0) != 0 || *(b + 0) != 0 || *(a + 0) != 255)
      {
	  fprintf (stderr, "Mismatching color #0\n");
	  return -11;
      }
    if (*(r + 1) != 0xab || *(g + 1) != 0xcd || *(b + 1) != 0xef
	|| *(a + 0) != 255)
      {
	  fprintf (stderr, "Mismatching color #1\n");
	  return -11;
      }
    rl2_free (r);
    rl2_free (g);
    rl2_free (b);
    rl2_free (a);

    if (rl2_set_palette_color (NULL, 0, 0, 0, 0, 255) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL palette set color\n");
	  return -12;
      }

    if (rl2_set_palette_color (palette, -1, 0, 0, 0, 255) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: palette set color (negative index)\n");
	  return -13;
      }

    if (rl2_set_palette_color (palette, 2, 0, 0, 0, 255) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: palette set color (exceeding index)\n");
	  return -14;
      }

    if (rl2_set_palette_hexrgb (NULL, 1, "#aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette HEX color\n");
	  return -15;
      }

    if (rl2_set_palette_hexrgb (palette, -1, "#aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unable to set palette HEX color (negative index)\n");
	  return -16;
      }

    if (rl2_set_palette_hexrgb (palette, 2, "#aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unable to set palette HEX color (exceeding index)\n");
	  return -17;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #1\n");
	  return -18;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "aBcDeF#") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #2\n");
	  return -19;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#zBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #3\n");
	  return -20;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#aBcDeZ") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #4\n");
	  return -21;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#aBcGef") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #5\n");
	  return -22;
      }

    if (rl2_set_palette_hexrgb (palette, 1, NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - NULL-hex\n");
	  return -23;
      }

    rl2_destroy_palette (palette);

    rl2_destroy_palette (NULL);

    if (rl2_get_palette_entries (NULL, &num_entries) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL palette # entries\n");
	  return -24;
      }

    if (rl2_get_palette_colors (NULL, &num_entries, &r, &g, &b, &a) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL palette - get colors\n");
	  return -25;
      }

    palette = create_rgb256_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 256-RGB\n");
	  return -26;
      }
    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-RGB - unexpected sample type %02x\n",
		   sample_type);
	  return -27;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-RGB - unexpected pixel type %02x\n",
		   pixel_type);
	  return -27;
      }
    rl2_destroy_palette (palette);

    palette = create_rgb16_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 16-RGB\n");
	  return -28;
      }
    if (sample_type != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-RGB - unexpected sample type %02x\n",
		   sample_type);
	  return -29;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-RGB - unexpected pixel type %02x\n",
		   pixel_type);
	  return -30;
      }
    rl2_destroy_palette (palette);

    palette = create_rgb4_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 4-RGB\n");
	  return -31;
      }
    if (sample_type != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-RGB - unexpected sample type %02x\n",
		   sample_type);
	  return -32;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-RGB - unexpected pixel type %02x\n",
		   pixel_type);
	  return -33;
      }
    rl2_destroy_palette (palette);

    palette = create_bicolor_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - BICOLOR\n");
	  return -34;
      }
    if (sample_type != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette BICOLOR - unexpected sample type %02x\n",
		   sample_type);
	  return -35;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette BICOLOR - unexpected pixel type %02x\n",
		   pixel_type);
	  return -36;
      }
    rl2_destroy_palette (palette);

    palette = create_gray256_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to get the Palette type - 256-GRAY\n");
	  return -37;
      }
    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-GRAY - unexpected sample type %02x\n",
		   sample_type);
	  return -38;
      }
    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-GRAY - unexpected pixel type %02x\n",
		   pixel_type);
	  return -39;
      }
    rl2_destroy_palette (palette);

    palette = create_gray16_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 16-GRAY\n");
	  return -40;
      }
    if (sample_type != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-GRAY - unexpected sample type %02x\n",
		   sample_type);
	  return -41;
      }
    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-GRAY - unexpected pixel type %02x\n",
		   pixel_type);
	  return -42;
      }
    rl2_destroy_palette (palette);

    palette = create_gray4_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 4-GRAY\n");
	  return -43;
      }
    if (sample_type != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-GRAY - unexpected sample type %02x\n",
		   sample_type);
	  return -44;
      }
    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-GRAY - unexpected pixel type %02x\n",
		   pixel_type);
	  return -45;
      }
    rl2_destroy_palette (palette);

    palette = create_monochrome_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to get the Palette type - MONOCHROME\n");
	  return -46;
      }
    if (sample_type != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette MONOCHROME - unexpected sample type %02x\n",
		   sample_type);
	  return -47;
      }
    if (pixel_type != RL2_PIXEL_MONOCHROME)
      {
	  fprintf (stderr,
		   "ERROR: palette MONOCHROME - unexpected pixel type %02x\n",
		   pixel_type);
	  return -48;
      }
    if (rl2_serialize_dbms_palette (palette, &blob, &blob_size) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to serialize a palette\n");
	  return -49;
      }
    rl2_destroy_palette (palette);
    palette = rl2_deserialize_dbms_palette (blob, blob_size);
    free (blob);
    if (palette == NULL)
      {
	  fprintf (stderr, "ERROR: unable to deserialize a palette\n");
	  return -50;
      }
    plt2 = rl2_clone_palette (palette);
    rl2_destroy_palette (palette);
    if (plt2 == NULL)
      {
	  fprintf (stderr, "ERROR: unable to clone a palette\n");
	  return 51;
      }
    rl2_destroy_palette (plt2);
    if (rl2_create_default_dbms_palette (&blob, &blob_size) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to create a default serialized palette\n");
	  return -52;
      }
    free (blob);
    if (rl2_serialize_dbms_palette (NULL, &blob, &blob_size) == RL2_OK)
      {
	  fprintf (stderr, "ERROR: unexpected NULL palette serialization\n");
	  return -53;
      }
    if (rl2_deserialize_dbms_palette (NULL, blob_size) != NULL)
      {
	  fprintf (stderr,
		   "ERROR: unexpected palette deserialization from NULL BLOB\n");
	  return -54;
      }
    if (rl2_deserialize_dbms_palette (blob, 0) != NULL)
      {
	  fprintf (stderr,
		   "ERROR: unexpected palette deserialization from zero-length BLOB\n");
	  return -55;
      }
    if (rl2_clone_palette (NULL) != NULL)
      {
	  fprintf (stderr, "ERROR: unexpected cloned NULL palette\n");
	  return 56;
      }

    return 0;
}
