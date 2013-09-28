/*

 rl2tiff -- TIFF / GeoTIFF related functions

 version 0.1, 2013 April 5

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

#include <webp/decode.h>
#include <webp/encode.h>

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

static rl2PrivTiffOriginPtr
create_tiff_origin (const char *path)
{
/* creating an uninitialized TIFF origin */
    int len;
    rl2PrivTiffOriginPtr origin;
    if (path == NULL)
	return NULL;
    origin = malloc (sizeof (rl2PrivTiffOrigin));
    if (origin == NULL)
	return NULL;

    len = strlen (path);
    origin->path = malloc (len + 1);
    strcpy (origin->path, path);
    origin->in = (TIFF *) 0;
    origin->tileWidth = 0;
    origin->tileHeight = 0;
    origin->rowsPerStrip = 0;
    origin->maxPalette = 0;
    origin->red = NULL;
    origin->green = NULL;
    origin->blue = NULL;
    origin->alpha = NULL;
    origin->Srid = -1;
    origin->srsName = NULL;
    origin->proj4text = NULL;
    return origin;
}

static int
alloc_palette (rl2PrivTiffOriginPtr tiff, int max_palette)
{
/* allocating an empty Palette */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    int i;

    if (origin == NULL)
	return 0;
    if (max_palette < 1 || max_palette > 256)
	return 0;
    origin->maxPalette = max_palette;
    origin->red = malloc (max_palette);
    if (origin->red == NULL)
	return 0;
    origin->green = malloc (max_palette);
    if (origin->green == NULL)
      {
	  free (origin->red);
	  return 0;
      }
    origin->blue = malloc (max_palette);
    if (origin->blue == NULL)
      {
	  free (origin->red);
	  free (origin->green);
	  return 0;
      }
    origin->alpha = malloc (max_palette);
    if (origin->alpha == NULL)
      {
	  free (origin->red);
	  free (origin->green);
	  free (origin->blue);
	  return 0;
      }
    for (i = 0; i < max_palette; i++)
      {
	  origin->red[i] = 0;
	  origin->green[i] = 0;
	  origin->blue[i] = 0;
	  origin->alpha[i] = 255;
      }
    return 1;
}

RL2_DECLARE void
rl2_destroy_tiff_origin (rl2TiffOriginPtr tiff)
{
/* memory cleanup - destroying a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return;
    if (origin->in != (TIFF *) 0)
	TIFFClose (origin->in);
    if (origin->path != NULL)
	free (origin->path);
    if (origin->red != NULL)
	free (origin->red);
    if (origin->green != NULL)
	free (origin->green);
    if (origin->blue != NULL)
	free (origin->blue);
    if (origin->alpha != NULL)
	free (origin->alpha);
    if (origin->srsName != NULL)
	free (origin->srsName);
    if (origin->proj4text != NULL)
	free (origin->proj4text);
    free (origin);
}

static void
geo_tiff_origin (const char *path, rl2PrivTiffOriginPtr origin)
{
/* attempting to retrieve georeferencing from a GeoTIFF origin */
    uint32 width = 0;
    uint32 height = 0;
    double cx;
    double cy;
    GTIFDefn definition;
    char *pString;
    int len;
    TIFF *in = (TIFF *) 0;
    GTIF *gtif = (GTIF *) 0;

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* reading from file */
    in = XTIFFOpen (path, "r");
    if (in == NULL)
	goto error;
    gtif = GTIFNew (in);
    if (gtif == NULL)
	goto error;

    if (!GTIFGetDefn (gtif, &definition))
	goto error;
/* retrieving the EPSG code */
    if (definition.PCS == 32767)
      {
	  if (definition.GCS != 32767)
	      origin->Srid = definition.GCS;
      }
    else
	origin->Srid = definition.PCS;
    if (origin->Srid < 0)
	goto error;
    if (definition.PCS == 32767)
      {
	  /* Get the GCS name if possible */
	  pString = NULL;
	  GTIFGetGCSInfo (definition.GCS, &pString, NULL, NULL, NULL);
	  if (pString != NULL)
	    {
		len = strlen (pString);
		origin->srsName = malloc (len + 1);
		strcpy (origin->srsName, pString);
		CPLFree (pString);
	    }
      }
    else
      {
	  /* Get the PCS name if possible */
	  pString = NULL;
	  GTIFGetPCSInfo (definition.PCS, &pString, NULL, NULL, NULL);
	  if (pString != NULL)
	    {
		len = strlen (pString);
		origin->srsName = malloc (len + 1);
		strcpy (origin->srsName, pString);
		CPLFree (pString);
	    }
      }
/* retrieving the PROJ.4 params */
    pString = GTIFGetProj4Defn (&definition);
    if (pString != NULL)
      {
	  len = strlen (pString);
	  origin->proj4text = malloc (len + 1);
	  strcpy (origin->proj4text, pString);
	  CPLFree (pString);
      }

/* retrieving the TIFF dimensions */
    TIFFGetField (in, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField (in, TIFFTAG_IMAGEWIDTH, &width);

/* computing the corners coords */
    cx = 0.0;
    cy = 0.0;
    GTIFImageToPCS (gtif, &cx, &cy);
    origin->minX = cx;
    origin->maxY = cy;
    cx = 0.0;
    cy = height;
    GTIFImageToPCS (gtif, &cx, &cy);
    origin->minY = cy;
    cx = width;
    cy = 0.0;
    GTIFImageToPCS (gtif, &cx, &cy);
    origin->maxX = cx;

/* computing the pixel resolution */
    origin->hResolution = (origin->maxX - origin->minX) / (double) width;
    origin->vResolution = (origin->maxY - origin->minY) / (double) height;

  error:
    if (in != (TIFF *) 0)
	XTIFFClose (in);
    if (gtif != (GTIF *) 0)
	GTIFFree (gtif);
}

RL2_DECLARE rl2TiffOriginPtr
rl2_create_tiff_origin (const char *path)
{
/* attempting to create a file-based TIFF / GeoTIFF origin */
    uint32 value32;
    uint16 value16;
    rl2PrivTiffOriginPtr origin = NULL;

    origin = create_tiff_origin (path);
    if (origin == NULL)
	return NULL;

/* attempting first to retrieve GeoTIFF georeferencing */
    geo_tiff_origin (path, origin);

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* reading from file */
    origin->in = TIFFOpen (path, "r");
    if (origin->in == NULL)
	goto error;
/* tiled/striped layout */
    origin->isTiled = TIFFIsTiled (origin->in);

/* retrieving TIFF dimensions */
    TIFFGetField (origin->in, TIFFTAG_IMAGELENGTH, &value32);
    origin->height = value32;
    TIFFGetField (origin->in, TIFFTAG_IMAGEWIDTH, &value32);
    origin->width = value32;
    if (origin->isTiled)
      {
	  TIFFGetField (origin->in, TIFFTAG_TILEWIDTH, &value32);
	  origin->tileWidth = value32;
	  TIFFGetField (origin->in, TIFFTAG_TILELENGTH, &value32);
	  origin->tileHeight = value32;
      }
    else
      {
	  TIFFGetField (origin->in, TIFFTAG_ROWSPERSTRIP, &value32);
	  origin->rowsPerStrip = value32;
      }

/* retrieving pixel configuration */
    TIFFGetField (origin->in, TIFFTAG_BITSPERSAMPLE, &value16);
    origin->bitsPerSample = value16;
    if (TIFFGetField (origin->in, TIFFTAG_SAMPLESPERPIXEL, &value16) == 0)
      {
	  /* attempting to recover badly formatted TIFFs */
	  origin->samplesPerPixel = 1;
      }
    else
	origin->samplesPerPixel = value16;
    TIFFGetField (origin->in, TIFFTAG_PHOTOMETRIC, &value16);
    origin->photometric = value16;
    TIFFGetField (origin->in, TIFFTAG_COMPRESSION, &value16);
    origin->compression = value16;
    if (TIFFGetField (origin->in, TIFFTAG_SAMPLEFORMAT, &value16) == 0)
	origin->sampleFormat = SAMPLEFORMAT_UINT;
    else
	origin->sampleFormat = value16;
    if (TIFFGetField (origin->in, TIFFTAG_PLANARCONFIG, &value16) == 0)
      {
	  /* attempting to recover badly formatted TIFFs */
	  origin->planarConfig = 1;
      }
    else
	origin->planarConfig = value16;
    if (origin->planarConfig != PLANARCONFIG_CONTIG)
	goto error;

    if (origin->photometric == 3 && origin->sampleFormat == SAMPLEFORMAT_UINT)
      {
	  /* attempting to retrieve a Palette */
	  uint16 *red;
	  uint16 *green;
	  uint16 *blue;
	  int max_palette = 0;
	  int i;
	  switch (origin->bitsPerSample)
	    {
	    case 1:
		max_palette = 2;
		break;
	    case 2:
		max_palette = 4;
		break;
	    case 3:
		max_palette = 8;
		break;
	    case 4:
		max_palette = 16;
		break;
	    case 5:
		max_palette = 32;
		break;
	    case 6:
		max_palette = 64;
		break;
	    case 7:
		max_palette = 128;
		break;
	    case 8:
		max_palette = 256;
		break;
	    };
	  if (max_palette == 0)
	      goto error;
	  if (!alloc_palette (origin, max_palette))
	      goto error;
	  if (TIFFGetField (origin->in, TIFFTAG_COLORMAP, &red, &green, &blue)
	      == 0)
	      goto error;
	  for (i = 0; i < max_palette; i++)
	    {
		if (red[i] < 256)
		    origin->red[i] = red[i];
		else
		    origin->red[i] = red[i] / 256;
		if (green[i] < 256)
		    origin->green[i] = green[i];
		else
		    origin->green[i] = green[i] / 256;
		if (blue[i] < 256)
		    origin->blue[i] = blue[i];
		else
		    origin->blue[i] = blue[i] / 256;
	    }
      }

    return (rl2TiffOriginPtr) origin;
  error:
    if (origin != NULL)
	rl2_destroy_tiff_origin ((rl2TiffOriginPtr) origin);
    return NULL;
}

RL2_DECLARE const char *
rl2_get_tiff_origin_path (rl2TiffOriginPtr tiff)
{
/* retrieving the input path from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return NULL;

    return origin->path;
}

RL2_DECLARE int
rl2_get_tiff_origin_size (rl2TiffOriginPtr tiff, unsigned short *width,
			  unsigned short *height)
{
/* retrieving Width and Height from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    *width = origin->width;
    *height = origin->height;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_srid (rl2TiffOriginPtr tiff, int *srid)
{
/* retrieving the SRID from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    *srid = origin->Srid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_extent (rl2TiffOriginPtr tiff, double *minX, double *minY,
			    double *maxX, double *maxY)
{
/* retrieving the Extent from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    *minX = origin->minX;
    *minY = origin->minY;
    *maxX = origin->maxX;
    *maxY = origin->maxY;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_resolution (rl2TiffOriginPtr tiff, double *hResolution,
				double *vResolution)
{
/* retrieving the Pixel Resolution from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    *hResolution = origin->hResolution;
    *vResolution = origin->vResolution;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_type (rl2TiffOriginPtr tiff, unsigned char *sample_type,
			  unsigned char *pixel_type, unsigned char *num_bands)
{
/* retrieving the sample/pixel type from a TIFF origin */
    int ok = 0;
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    /*
       if (origin->isTiled)
       {
       fprintf(stderr, "tileWidth=%d\n", origin->tileWidth);
       fprintf(stderr, "tileHeight=%d\n", origin->tileHeight);
       }
       else
       fprintf(stderr, "rowPerStrip=%d\n", origin->rowsPerStrip);
       fprintf(stderr, "bitsPerSample=%d\n", origin->bitsPerSample);
       fprintf(stderr, "samplesPerPixel=%d\n", origin->samplesPerPixel);
       fprintf(stderr, "photometric=%d\n", origin->photometric);
       fprintf(stderr, "compression=%d\n", origin->compression);
       fprintf(stderr, "sampleFormat=%d\n", origin->sampleFormat);
       fprintf(stderr, "maxPalette=%d\n", origin->maxPalette);
       if (origin->Srid > 0)
       {
       fprintf(stderr, "Srid=%d\n", origin->Srid);
       fprintf(stderr, "SRS=%s\n", origin->srsName);
       fprintf(stderr, "proj=%s\n", origin->proj4text);
       fprintf(stderr, "hRes=%1.6f vRes=%1.6f\n", origin->hResolution, origin->vResolution);
       fprintf(stderr, "%1.6f %1.6f %1.6f %1.6f\n", origin->minX, origin->minY, origin->maxX, origin->maxY);
       }
     */

    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 1 && origin->photometric < 2)
      {
	  /* could be some kind of MONOCHROME */
	  if (origin->bitsPerSample == 1)
	    {
		*sample_type = RL2_SAMPLE_1_BIT;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_MONOCHROME;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 1 && origin->photometric < 2)
      {
	  /* could be some kind of GRAYSCALE */
	  if (origin->bitsPerSample == 2)
	    {
		*sample_type = RL2_SAMPLE_2_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 4)
	    {
		*sample_type = RL2_SAMPLE_4_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 16)
	    {
		*sample_type = RL2_SAMPLE_UINT16;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_GRAYSCALE;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 1 && origin->photometric == 3)
      {
	  /* could be some kind of PALETTE */
	  if (origin->bitsPerSample == 1)
	    {
		*sample_type = RL2_SAMPLE_1_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 2)
	    {
		*sample_type = RL2_SAMPLE_2_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 4)
	    {
		*sample_type = RL2_SAMPLE_4_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_PALETTE;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 3 && origin->photometric == 2)
      {
	  /* could be some kind of RGB */
	  if (origin->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 16)
	    {
		*sample_type = RL2_SAMPLE_UINT16;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_RGB;
		*num_bands = 3;
		return RL2_OK;
	    }
      }
    if (origin->samplesPerPixel == 1 && origin->photometric < 2)
      {
	  /* could be some kind of DATA-GRID */
	  if (origin->sampleFormat == SAMPLEFORMAT_INT)
	    {
		/* Signed Integer */
		if (origin->bitsPerSample == 8)
		  {
		      *sample_type = RL2_SAMPLE_INT8;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 16)
		  {
		      *sample_type = RL2_SAMPLE_INT16;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_INT32;
		      ok = 1;
		  }
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT)
	    {
		/* Unsigned Integer */
		if (origin->bitsPerSample == 8)
		  {
		      *sample_type = RL2_SAMPLE_UINT8;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 16)
		  {
		      *sample_type = RL2_SAMPLE_UINT16;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_UINT32;
		      ok = 1;
		  }
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP)
	    {
		/* Floating-Point */
		if (origin->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_FLOAT;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 64)
		  {
		      *sample_type = RL2_SAMPLE_DOUBLE;
		      ok = 1;
		  }
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_DATAGRID;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_eval_tiff_origin_compatibility (rl2CoveragePtr cvg, rl2TiffOriginPtr tiff)
{
/* testing if a Coverage and a TIFF origin are mutually compatible */
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;

    if (coverage == NULL || tiff == NULL)
	return RL2_ERROR;
    if (rl2_get_tiff_origin_type (tiff, &sample_type, &pixel_type, &num_bands)
	!= RL2_OK)
	return RL2_ERROR;

    if (coverage->sampleType != sample_type)
	return RL2_FALSE;
    if (coverage->pixelType != pixel_type)
	return RL2_FALSE;
    if (coverage->nBands != num_bands)
	return RL2_FALSE;
    return RL2_TRUE;
}

static int
read_RGBA_tiles (rl2PrivTiffOriginPtr origin, unsigned short width,
		 unsigned short height, unsigned char pixel_type,
		 unsigned char num_bands, unsigned int startRow,
		 unsigned int startCol, unsigned char *pixels,
		 rl2PalettePtr palette)
{
/* reading TIFF RGBA tiles */
    uint32 tile_x;
    uint32 tile_y;
    uint32 x;
    uint32 y;
    uint32 pix;
    uint32 *tiff_tile = NULL;
    uint32 *p_in;
    unsigned char *p_out;
    unsigned int dest_x;
    unsigned int dest_y;
    int skip;
    unsigned char index;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;

    tiff_tile =
	malloc (sizeof (uint32) * origin->tileWidth * origin->tileHeight);
    if (tiff_tile == NULL)
	goto error;

    for (tile_y = 0; tile_y < origin->height; tile_y += origin->tileHeight)
      {
	  /* scanning tiles by row */
	  unsigned int tiff_min_y = tile_y;
	  unsigned int tiff_max_y = tile_y + origin->tileHeight - 1;
	  skip = 0;
	  if (tiff_min_y > (startRow + height))
	      skip = 1;
	  if (tiff_max_y < startRow)
	      skip = 1;
	  if (skip)
	    {
		/* skipping any not required tile */
		continue;
	    }
	  for (tile_x = 0; tile_x < origin->width; tile_x += origin->tileWidth)
	    {
		/* reading a TIFF tile */
		unsigned int tiff_min_x = tile_x;
		unsigned int tiff_max_x = tile_x + origin->tileWidth - 1;
		skip = 0;
		if (tiff_min_x > (startCol + width))
		    skip = 1;
		if (tiff_max_x < startCol)
		    skip = 1;
		if (skip)
		  {
		      /* skipping any not required tile */
		      continue;
		  }
		if (!TIFFReadRGBATile (origin->in, tile_x, tile_y, tiff_tile))
		    goto error;
		for (y = 0; y < origin->tileHeight; y++)
		  {
		      dest_y = tile_y + (origin->tileHeight - y) - 1;
		      if (dest_y < startRow || dest_y >= (startRow + height))
			  continue;
		      p_in = tiff_tile + (origin->tileWidth * y);
		      for (x = 0; x < origin->tileWidth; x++)
			{
			    dest_x = tile_x + x;
			    if (dest_x < startCol
				|| dest_x >= (startCol + width))
			      {
				  p_in++;
				  continue;
			      }
			    p_out =
				pixels +
				((dest_y - startRow) * width * num_bands) +
				((dest_x - startCol) * num_bands);
			    pix = *p_in++;
			    red = TIFFGetR (pix);
			    green = TIFFGetG (pix);
			    blue = TIFFGetB (pix);
			    alpha = TIFFGetA (pix);
			    if (pixel_type == RL2_PIXEL_PALETTE)
			      {
				  /* PALETTE image */
				  if (rl2_get_palette_index
				      (palette, &index, red, green,
				       blue, alpha) != RL2_OK)
				      index = 0;
				  *p_out++ = index;
			      }
			    else if (pixel_type == RL2_PIXEL_MONOCHROME)
			      {
				  /* MONOCHROME  image */
				  index = red;
				  if (index != 0)
				      index = 1;
				  if (origin->photometric == 1)
				    {
					/* BlackIsZero - inverting black and white indexes */
					if (index == 0)
					    index = 1;
					else
					    index = 0;
				    }
				  *p_out++ = index;
			      }
			    else if (pixel_type == RL2_PIXEL_GRAYSCALE)
			      {
				  /* GRAYSCALE  image */
				  index = red;
				  if (origin->photometric == 0)
				    {
					/* WhiteIsZero - inverting the grayscale */
					index = 255 - red;
				    }
				  *p_out++ = index;
			      }
			    else
			      {
				  /* should be an RGB image */
				  *p_out++ = red;
				  *p_out++ = green;
				  *p_out++ = blue;
			      }
			}
		  }
	    }
      }

    free (tiff_tile);
    return RL2_OK;
  error:
    if (tiff_tile != NULL)
	free (tiff_tile);
    return RL2_ERROR;
}

static int
read_RGBA_strips (rl2PrivTiffOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned char pixel_type,
		  unsigned char num_bands, unsigned int startRow,
		  unsigned int startCol, unsigned char *pixels,
		  rl2PalettePtr palette)
{
/* reading TIFF RGBA strips */
    uint32 strip;
    uint32 x;
    uint32 y;
    uint32 pix;
    uint32 *tiff_strip = NULL;
    uint32 *p_in;
    unsigned char *p_out;
    unsigned int dest_x;
    unsigned int dest_y;
    int skip;
    unsigned char index;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;

    tiff_strip =
	malloc (sizeof (uint32) * origin->width * origin->rowsPerStrip);
    if (tiff_strip == NULL)
	goto error;

    for (strip = 0; strip < origin->height; strip += origin->rowsPerStrip)
      {
	  /* scanning strips by row */
	  unsigned int tiff_min_y = strip;
	  unsigned int tiff_max_y = strip + origin->rowsPerStrip - 1;
	  skip = 0;
	  if (tiff_min_y > (startRow + height))
	      skip = 1;
	  if (tiff_max_y < startRow)
	      skip = 1;
	  if (skip)
	    {
		/* skipping any not required strip */
		continue;
	    }
	  if (!TIFFReadRGBAStrip (origin->in, strip, tiff_strip))
	      goto error;
	  for (y = 0; y < origin->rowsPerStrip; y++)
	    {
		dest_y = strip + (origin->rowsPerStrip - y) - 1;
		if (dest_y < startRow || dest_y >= (startRow + height))
		    continue;
		p_in = tiff_strip + (origin->width * y);
		for (x = 0; x < origin->width; x++)
		  {
		      dest_x = x;
		      if (dest_x < startCol || dest_x >= (startCol + width))
			{
			    p_in++;
			    continue;
			}
		      p_out =
			  pixels + ((dest_y - startRow) * width * num_bands) +
			  ((dest_x - startCol) * num_bands);
		      pix = *p_in++;
		      red = TIFFGetR (pix);
		      green = TIFFGetG (pix);
		      blue = TIFFGetB (pix);
		      alpha = TIFFGetA (pix);
		      if (pixel_type == RL2_PIXEL_PALETTE)
			{
			    /* PALETTE image */
			    if (rl2_get_palette_index
				(palette, &index, red, green, blue,
				 alpha) != RL2_OK)
				index = 0;
			    *p_out++ = index;
			}
		      else if (pixel_type == RL2_PIXEL_MONOCHROME)
			{
			    /* MONOCHROME  image */
			    index = red;
			    if (index != 0)
				index = 1;
			    if (origin->photometric == 1)
			      {
				  /* BlackIsZero - inverting black and white indexes */
				  if (index == 0)
				      index = 1;
				  else
				      index = 0;
			      }
			    *p_out++ = index;
			}
		      else if (pixel_type == RL2_PIXEL_GRAYSCALE)
			{
			    /* GRAYSCALE  image */
			    index = red;
			    if (origin->photometric == 0)
			      {
				  /* WhiteIsZero - inverting the grayscale */
				  index = 255 - red;
			      }
			    *p_out++ = index;
			}
		      else
			{
			    /* should be an RGB image */
			    *p_out++ = red;
			    *p_out++ = green;
			    *p_out++ = blue;
			}
		  }
	    }
      }

    free (tiff_strip);
    return RL2_OK;
  error:
    if (tiff_strip != NULL)
	free (tiff_strip);
    return RL2_ERROR;
}

static int
read_from_tiff (rl2PrivTiffOriginPtr origin, unsigned short width,
		unsigned short height, unsigned char sample_type,
		unsigned char pixel_type, unsigned char num_bands,
		unsigned int startRow, unsigned int startCol,
		unsigned char **pixels, int *pixels_sz, rl2PalettePtr palette)
{
/* creating a tile from the Tiff origin */
    int ret;
    unsigned char *bufPixels = NULL;
    int bufPixelsSz = 0;
    int pix_sz = 1;

/* allocating the pixels buffer */
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
    bufPixelsSz = width * height * pix_sz * num_bands;
    bufPixels = malloc (bufPixelsSz);
    if (bufPixels == NULL)
	goto error;
    if ((startRow + height) > origin->height
	|| (startCol + width) > origin->width)
	rl2_prime_void_tile (bufPixels, width, height, sample_type, num_bands);

    if (origin->bitsPerSample <= 8 && origin->sampleFormat == SAMPLEFORMAT_UINT
	&& (origin->samplesPerPixel == 1 || origin->samplesPerPixel == 3))
      {
	  /* using the TIFF RGBA methods */
	  if (origin->isTiled)
	      ret =
		  read_RGBA_tiles (origin, width, height, pixel_type, num_bands,
				   startRow, startCol, bufPixels, palette);
	  else
	      ret =
		  read_RGBA_strips (origin, width, height, pixel_type,
				    num_bands, startRow, startCol, bufPixels,
				    palette);
	  if (ret != RL2_OK)
	      goto error;
      }

    *pixels = bufPixels;
    *pixels_sz = bufPixelsSz;
    return RL2_OK;
  error:
    if (bufPixels != NULL)
	free (bufPixels);
    return RL2_ERROR;
}

RL2_DECLARE rl2RasterPtr
rl2_get_tile_from_tiff_origin (rl2CoveragePtr cvg, rl2TiffOriginPtr tiff,
			       unsigned int startRow, unsigned int startCol)
{
/* attempting to create a Coverage-tile from a Tiff origin */
    unsigned int x;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    int unused_width = 0;
    int unused_height = 0;

    if (coverage == NULL || tiff == NULL)
	return NULL;
    if (rl2_eval_tiff_origin_compatibility (cvg, tiff) != RL2_TRUE)
	return NULL;

/* testing for tile's boundary validity */
    if (startCol > origin->width)
	return NULL;
    if (startRow > origin->height)
	return NULL;
    x = startCol / coverage->tileWidth;
    if ((x * coverage->tileWidth) != startCol)
	return NULL;
    x = startRow / coverage->tileHeight;
    if ((x * coverage->tileHeight) != startRow)
	return NULL;

    if (origin->photometric == 3)
      {
	  /* creating a Palette */
	  palette = rl2_create_palette (origin->maxPalette);
	  for (x = 0; x < origin->maxPalette; x++)
	      rl2_set_palette_color (palette, x, origin->red[x],
				     origin->green[x], origin->blue[x],
				     origin->alpha[x]);
      }

/* attempting to create the tile */
    if (read_from_tiff
	(origin, coverage->tileWidth, coverage->tileHeight,
	 coverage->sampleType, coverage->pixelType, coverage->nBands, startRow,
	 startCol, &pixels, &pixels_sz, palette) != RL2_OK)
	goto error;
    if (startCol + coverage->tileWidth > origin->width)
	unused_width = (startCol + coverage->tileWidth) - origin->width;
    if (startRow + coverage->tileHeight > origin->height)
	unused_height = (startRow + coverage->tileHeight) - origin->height;
    if (unused_width || unused_height)
      {
	  /* 
	   * creating a Transparency Mask so to shadow any 
	   * unused portion of the current tile 
	   */
	  int shadow_x = coverage->tileWidth - unused_width;
	  int shadow_y = coverage->tileHeight - unused_height;
	  int row;
	  mask_size = coverage->tileWidth * coverage->tileHeight;
	  mask = malloc (mask_size);
	  if (mask == NULL)
	      goto error;
	  for (row = 0; row < coverage->tileHeight; row++)
	    {
		unsigned char *p = mask + (row * coverage->tileWidth);
		if (row >= shadow_y)
		  {
		      /* full transparent scanline */
		      memset (p, 0, coverage->tileWidth);
		  }
		else
		  {
		      /* partially transparent scanline */
		      int col;
		      for (col = 0; col < coverage->tileWidth; col++)
			{
			    if (col >= shadow_x)
			      {
				  /* transparent pixel */
				  *p++ = 0;
			      }
			    else
			      {
				  /* opaque pixel */
				  *p++ = 1;
			      }
			}
		  }
	    }
      }
    raster =
	rl2_create_raster (coverage->tileWidth, coverage->tileHeight,
			   coverage->sampleType, coverage->pixelType,
			   coverage->nBands, pixels, pixels_sz, palette, mask,
			   mask_size, NULL);
    if (raster == NULL)
	fprintf (stderr, "merfissima\n");
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
