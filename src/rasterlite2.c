/*

 rasterlite2 -- main public functions

 version 0.1, 2013 March 29

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

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

static int
is_valid_sample_type (unsigned char sample_type)
{
/* checking a sample-type for validity */
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
      case RL2_SAMPLE_DOUBLE:
	  return 1;
      };
    return 0;
}

static int
is_valid_pixel_type (unsigned char pixel_type)
{
/* checking a pixel-type for validity */
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
      case RL2_PIXEL_MULTIBAND:
      case RL2_PIXEL_DATAGRID:
	  return 1;
      };
    return 0;
}

static int
is_valid_compression (unsigned char compression)
{
/* checking a compression-type for validity */
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
      case RL2_COMPRESSION_DEFLATE:
      case RL2_COMPRESSION_LZMA:
      case RL2_COMPRESSION_GIF:
      case RL2_COMPRESSION_PNG:
      case RL2_COMPRESSION_JPEG:
      case RL2_COMPRESSION_LOSSY_WEBP:
      case RL2_COMPRESSION_LOSSLESS_WEBP:
      case RL2_COMPRESSION_CCITTFAX4:
	  return 1;
      };
    return 0;
}

static int
check_coverage_self_consistency (unsigned char sample_type,
				 unsigned char pixel_type,
				 unsigned char num_samples,
				 unsigned char compression)
{
/* checking overall self-consistency for coverage params */
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
	  if (sample_type != RL2_SAMPLE_1_BIT || num_samples != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_PNG:
	    case RL2_COMPRESSION_CCITTFAX4:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_GIF:
	    case RL2_COMPRESSION_PNG:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_GIF:
	    case RL2_COMPRESSION_PNG:
	    case RL2_COMPRESSION_JPEG:
	    case RL2_COMPRESSION_LOSSY_WEBP:
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_RGB:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 3)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_PNG:
	    case RL2_COMPRESSION_JPEG:
	    case RL2_COMPRESSION_LOSSY_WEBP:
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
		break;
	    default:
		return 0;
	    };
	  if (num_samples < 2)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_LZMA:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
	    case RL2_SAMPLE_FLOAT:
	    case RL2_SAMPLE_DOUBLE:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_LZMA:
		break;
	    default:
		return 0;
	    };
	  break;
      };
    return 1;
}

static int
check_raster_self_consistency (unsigned char sample_type,
			       unsigned char pixel_type,
			       unsigned char num_samples)
{
/* checking overall self-consistency for raster params */
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
	  if (sample_type != RL2_SAMPLE_1_BIT || num_samples != 1)
	      return 0;
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  break;
      case RL2_PIXEL_RGB:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 3)
	      return 0;
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
		break;
	    default:
		return 0;
	    };
	  if (num_samples < 2)
	      return 0;
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
	    case RL2_SAMPLE_FLOAT:
	    case RL2_SAMPLE_DOUBLE:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  break;
      };
    return 1;
}

RL2_DECLARE void
rl2_free (void *ptr)
{
/* memory cleanup - generic free */
    if (ptr != NULL)
	free (ptr);
}

static int
check_coverage_no_data (rl2PrivPixelPtr pxl_no_data, unsigned char sample_type,
			unsigned char pixel_type, unsigned char num_samples)
{
/* checking if the NoData pixel is consistent with the Coverage */
    if (pxl_no_data == NULL)
	return 1;
    if (pxl_no_data->sampleType != sample_type)
	return 0;
    if (pxl_no_data->pixelType != pixel_type)
	return 0;
    if (pxl_no_data->nBands != num_samples)
	return 0;
    return 1;
}

RL2_DECLARE rl2CoveragePtr
rl2_create_coverage (const char *name, unsigned char sample_type,
		     unsigned char pixel_type, unsigned char num_samples,
		     unsigned char compression, int quality,
		     unsigned short tile_width, unsigned short tile_height,
		     rl2PixelPtr no_data)
{
/* allocating and initializing a Coverage object */
    int len;
    rl2PrivPixelPtr pxl_no_data = (rl2PrivPixelPtr) no_data;
    rl2PrivCoveragePtr cvg = NULL;
    if (name == NULL)
	return NULL;
    if (!is_valid_sample_type (sample_type))
	return NULL;
    if (!is_valid_pixel_type (pixel_type))
	return NULL;
    if (!is_valid_compression (compression))
	return NULL;
    if (!check_coverage_self_consistency
	(sample_type, pixel_type, num_samples, compression))
	return NULL;
    if (tile_width < 256 || tile_width > 1024)
	return NULL;
    if (tile_height < 256 || tile_height > 1024)
	return NULL;
    if ((tile_width % 16) != 0)
	return NULL;
    if ((tile_height % 16) != 0)
	return NULL;
    if (!check_coverage_no_data
	(pxl_no_data, sample_type, pixel_type, num_samples))
	return NULL;

    cvg = malloc (sizeof (rl2PrivCoverage));
    if (cvg == NULL)
	return NULL;
    len = strlen (name);
    cvg->coverageName = malloc (len + 1);
    strcpy (cvg->coverageName, name);
    cvg->sampleType = sample_type;
    cvg->pixelType = pixel_type;
    cvg->nBands = num_samples;
    cvg->Compression = compression;
    if (quality < 0)
	cvg->Quality = 0;
    else if (quality > 100)
	cvg->Quality = 100;
    else
	cvg->Quality = quality;
    cvg->tileWidth = tile_width;
    cvg->tileHeight = tile_height;
    cvg->Srid = RL2_GEOREFERENCING_NONE;
    cvg->noData = pxl_no_data;
    return (rl2CoveragePtr) cvg;
}

RL2_DECLARE void
rl2_destroy_coverage (rl2CoveragePtr ptr)
{
/* memory cleanup - destroying a Coverage object */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return;
    if (cvg->coverageName != NULL)
	free (cvg->coverageName);
    if (cvg->noData != NULL)
	rl2_destroy_pixel ((rl2PixelPtr) (cvg->noData));
    free (cvg);
}

RL2_DECLARE int
rl2_coverage_georeference (rl2CoveragePtr ptr, int srid, double horz_res,
			   double vert_res)
{
/* setting the Coverage's georeferencing infos */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    cvg->Srid = srid;
    cvg->hResolution = horz_res;
    cvg->vResolution = vert_res;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_get_coverage_name (rl2CoveragePtr ptr)
{
/* return the Coverage name */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return NULL;
    return cvg->coverageName;
}

RL2_DECLARE int
rl2_get_coverage_type (rl2CoveragePtr ptr, unsigned char *sample_type,
		       unsigned char *pixel_type, unsigned char *num_bands)
{
/* return the Coverage type */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    *sample_type = cvg->sampleType;
    *pixel_type = cvg->pixelType;
    *num_bands = cvg->nBands;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_coverage_compression (rl2CoveragePtr ptr, unsigned char *compression,
			      int *quality)
{
/* return the Coverage compression */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    *compression = cvg->Compression;
    *quality = cvg->Quality;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_coverage_uncompressed (rl2CoveragePtr ptr, int *is_uncompressed)
{
/* tests if Coverage is compressed */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    if (cvg->Compression == RL2_COMPRESSION_NONE)
	*is_uncompressed = RL2_TRUE;
    else
	*is_uncompressed = RL2_FALSE;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_coverage_compression_lossless (rl2CoveragePtr ptr, int *is_lossless)
{
/* tests if Coverage is lossless compressed */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    switch (cvg->Compression)
      {
      case RL2_COMPRESSION_DEFLATE:
      case RL2_COMPRESSION_LZMA:
      case RL2_COMPRESSION_GIF:
      case RL2_COMPRESSION_PNG:
	  *is_lossless = RL2_TRUE;
	  break;
      default:
	  *is_lossless = RL2_FALSE;
	  break;
      };
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_coverage_compression_lossy (rl2CoveragePtr ptr, int *is_lossy)
{
/* tests if Coverage is lossy compressed */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    switch (cvg->Compression)
      {
      case RL2_COMPRESSION_JPEG:
      case RL2_COMPRESSION_LOSSY_WEBP:
	  *is_lossy = RL2_TRUE;
	  break;
      default:
	  *is_lossy = RL2_FALSE;
	  break;
      };
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_coverage_tile_size (rl2CoveragePtr ptr, unsigned short *tile_width,
			    unsigned short *tile_height)
{
/* return the Coverage tile width */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    *tile_width = cvg->tileWidth;
    *tile_height = cvg->tileHeight;
    return RL2_OK;
}

RL2_DECLARE rl2PixelPtr
rl2_get_coverage_no_data (rl2CoveragePtr ptr)
{
/* return the Coverage NoData pixel (if any) */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return NULL;
    return (rl2PixelPtr) (cvg->noData);
}

RL2_DECLARE rl2PixelPtr
rl2_create_coverage_pixel (rl2CoveragePtr ptr)
{
/* creating a Pixel matching the given Coverage */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return NULL;
    return rl2_create_pixel (cvg->sampleType, cvg->pixelType, cvg->nBands);
}

RL2_DECLARE int
rl2_get_coverage_srid (rl2CoveragePtr ptr, int *srid)
{
/* return the Coverage SRID */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) ptr;
    if (cvg == NULL)
	return RL2_ERROR;
    *srid = cvg->Srid;
    return RL2_OK;
}

RL2_DECLARE rl2SectionPtr
rl2_create_section (const char *name, unsigned char compression,
		    unsigned short tile_width, unsigned short tile_height,
		    rl2RasterPtr rst)
{
/* allocating and initializing a Section object */
    int len;
    rl2PrivRasterPtr raster = (rl2PrivRasterPtr) rst;
    rl2PrivSectionPtr scn = NULL;
    if (name == NULL)
	return NULL;
    if (raster == NULL)
	return NULL;
    if (!check_coverage_self_consistency
	(raster->sampleType, raster->pixelType, raster->nBands, compression))
	return NULL;
    if (tile_width == RL2_TILESIZE_UNDEFINED
	&& tile_height == RL2_TILESIZE_UNDEFINED)
	;
    else
      {
	  if (tile_width < 256 || tile_width > 1024)
	      return NULL;
	  if (tile_height < 256 || tile_height > 1024)
	      return NULL;
	  if ((tile_width % 16) != 0)
	      return NULL;
	  if ((tile_height % 16) != 0)
	      return NULL;
      }

    scn = malloc (sizeof (rl2PrivSection));
    if (scn == NULL)
	return NULL;
    len = strlen (name);
    scn->sectionName = malloc (len + 1);
    strcpy (scn->sectionName, name);
    scn->Compression = compression;
    scn->tileWidth = tile_width;
    scn->tileHeight = tile_height;
    scn->Raster = raster;
    return (rl2SectionPtr) scn;
}

RL2_DECLARE void
rl2_destroy_section (rl2SectionPtr ptr)
{
/* memory cleanup - destroying a Section object */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return;
    if (scn->sectionName != NULL)
	free (scn->sectionName);
    if (scn->Raster != NULL)
	rl2_destroy_raster ((rl2RasterPtr) (scn->Raster));
    free (scn);
}

RL2_DECLARE const char *
rl2_get_section_name (rl2SectionPtr ptr)
{
/* return the Section name */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return NULL;
    return scn->sectionName;
}

RL2_DECLARE int
rl2_get_section_compression (rl2SectionPtr ptr, unsigned char *compression)
{
/* return the Section compression */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return RL2_ERROR;
    *compression = scn->Compression;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_section_uncompressed (rl2SectionPtr ptr, int *is_uncompressed)
{
/* tests if Section is compressed */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return RL2_ERROR;
    if (scn->Compression == RL2_COMPRESSION_NONE)
	*is_uncompressed = RL2_TRUE;
    else
	*is_uncompressed = RL2_FALSE;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_section_compression_lossless (rl2SectionPtr ptr, int *is_lossless)
{
/* tests if Section is lossless compressed */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return RL2_ERROR;
    switch (scn->Compression)
      {
      case RL2_COMPRESSION_DEFLATE:
      case RL2_COMPRESSION_LZMA:
      case RL2_COMPRESSION_GIF:
      case RL2_COMPRESSION_PNG:
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  *is_lossless = RL2_TRUE;
	  break;
      default:
	  *is_lossless = RL2_FALSE;
	  break;
      };
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_section_compression_lossy (rl2SectionPtr ptr, int *is_lossy)
{
/* tests if Section is lossy compressed */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return RL2_ERROR;
    switch (scn->Compression)
      {
      case RL2_COMPRESSION_JPEG:
      case RL2_COMPRESSION_LOSSY_WEBP:
	  *is_lossy = RL2_TRUE;
	  break;
      default:
	  *is_lossy = RL2_FALSE;
	  break;
      };
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_section_tile_size (rl2SectionPtr ptr, unsigned short *tile_width,
			   unsigned short *tile_height)
{
/* return the Section tile width */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return RL2_ERROR;
    *tile_width = scn->tileWidth;
    *tile_height = scn->tileHeight;
    return RL2_OK;
}

RL2_DECLARE rl2RasterPtr
rl2_get_section_raster (rl2SectionPtr ptr)
{
/* return the Section Raster */
    rl2PrivSectionPtr scn = (rl2PrivSectionPtr) ptr;
    if (scn == NULL)
	return NULL;
    return (rl2RasterPtr) (scn->Raster);
}

int
compute_raster_buffer_size (unsigned short width, unsigned short height,
			    unsigned char sample_type,
			    unsigned char num_samples)
{
/* determining the pixel buffer size */
    int pixel_size = 1;
    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pixel_size = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pixel_size = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pixel_size = 8;
	  break;
      };
    return width * height * pixel_size * num_samples;
}

static int
check_raster_no_data (rl2PrivPixelPtr pxl_no_data, unsigned char sample_type,
		      unsigned char pixel_type, unsigned char num_samples)
{
/* checking if the NoData pixel is consistent with the Raster */
    if (pxl_no_data == NULL)
	return 1;
    if (pxl_no_data->sampleType != sample_type)
	return 0;
    if (pxl_no_data->pixelType != pixel_type)
	return 0;
    if (pxl_no_data->nBands != num_samples)
	return 0;
    return 1;
}

RL2_DECLARE rl2RasterPtr
rl2_create_raster (unsigned short width, unsigned short height,
		   unsigned char sample_type, unsigned char pixel_type,
		   unsigned char num_samples, unsigned char *bufpix,
		   int bufpix_size, rl2PalettePtr palette, unsigned char *mask,
		   int mask_size, rl2PixelPtr no_data)
{
/* allocating and initializing a Raster object */
    rl2PrivPixelPtr pxl_no_data = (rl2PrivPixelPtr) no_data;
    unsigned char *p;
    int row;
    int col;
    int raster_size;
    rl2PrivRasterPtr rst = NULL;

    if (!is_valid_sample_type (sample_type))
	return NULL;
    if (!is_valid_pixel_type (pixel_type))
	return NULL;
    if (!check_raster_self_consistency (sample_type, pixel_type, num_samples))
	return NULL;
    if (width < 1 || height < 1)
	return NULL;
    raster_size =
	compute_raster_buffer_size (width, height, sample_type, num_samples);
    if (bufpix == NULL)
	return NULL;
    if (raster_size != bufpix_size)
	return NULL;
    if (pixel_type == RL2_PIXEL_PALETTE && palette == NULL)
	return NULL;
    if (palette != NULL && pixel_type != RL2_PIXEL_PALETTE)
	return NULL;
    if (!check_raster_no_data
	(pxl_no_data, sample_type, pixel_type, num_samples))
	return NULL;
    if (mask != NULL)
      {
	  /* checking the mask size */
	  if (width * height != mask_size)
	      return NULL;
	  p = mask;
	  for (row = 0; row < height; row++)
	    {
		/* checking if sample doesn't exceed the max value */
		for (col = 0; col < width; col++)
		  {
		      if (*p++ > 1)
			  return NULL;
		  }
	    }
      }
    if (palette != NULL)
      {
	  /* checking the Palette for validity */
	  unsigned short max_palette;
	  rl2_get_palette_entries (palette, &max_palette);
	  p = bufpix;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      if (*p++ >= max_palette)
			  return NULL;
		  }
	    }
      }
    switch (sample_type)
      {
	  /* checking if sample doesn't exceed the max value for #bits */
      case RL2_SAMPLE_1_BIT:
	  p = bufpix;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      if (*p++ > 1)
			  return NULL;
		  }
	    }
	  break;
      case RL2_SAMPLE_2_BIT:
	  p = bufpix;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      if (*p++ > 3)
			  return NULL;
		  }
	    }
	  break;
      case RL2_SAMPLE_4_BIT:
	  p = bufpix;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      if (*p++ > 15)
			  return NULL;
		  }
	    }
	  break;
      };

    rst = malloc (sizeof (rl2PrivRaster));
    if (rst == NULL)
	return NULL;
    rst->sampleType = sample_type;
    rst->pixelType = pixel_type;
    rst->nBands = num_samples;
    rst->width = width;
    rst->height = height;
    rst->Srid = RL2_GEOREFERENCING_NONE;
    rst->minX = 0.0;
    rst->minY = 0.0;
    rst->maxX = width;
    rst->maxY = height;
    rst->rasterBuffer = bufpix;
    rst->maskBuffer = mask;
    rst->Palette = (rl2PrivPalettePtr) palette;
    rst->noData = pxl_no_data;
    return (rl2RasterPtr) rst;
}

RL2_DECLARE void
rl2_destroy_raster (rl2RasterPtr ptr)
{
/* memory cleanup - destroying a Raster object */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return;
    if (rst->rasterBuffer != NULL)
	free (rst->rasterBuffer);
    if (rst->maskBuffer != NULL)
	free (rst->maskBuffer);
    if (rst->Palette != NULL)
	rl2_destroy_palette ((rl2PalettePtr) (rst->Palette));
    if (rst->noData != NULL)
	rl2_destroy_pixel ((rl2PixelPtr) (rst->noData));
    free (rst);
}

RL2_DECLARE rl2PixelPtr
rl2_get_raster_no_data (rl2RasterPtr ptr)
{
/* return the Raster NoData pixel (if any) */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return NULL;
    return (rl2PixelPtr) (rst->noData);
}

RL2_DECLARE int
rl2_get_raster_type (rl2RasterPtr ptr, unsigned char *sample_type,
		     unsigned char *pixel_type, unsigned char *num_bands)
{
/* return the Raster type */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return RL2_ERROR;
    *sample_type = rst->sampleType;
    *pixel_type = rst->pixelType;
    *num_bands = rst->nBands;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_size (rl2RasterPtr ptr, unsigned short *width,
		     unsigned short *height)
{
/* return the Raster width */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return RL2_ERROR;
    *width = rst->width;
    *height = rst->height;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_srid (rl2RasterPtr ptr, int *srid)
{
/* return the Raster SRID */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return RL2_ERROR;
    *srid = rst->Srid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_extent (rl2RasterPtr ptr, double *minX, double *minY,
		       double *maxX, double *maxY)
{
/* return the Raster Min X coord */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return RL2_ERROR;
    if (rst->Srid == RL2_GEOREFERENCING_NONE)
      {
	  *minX = 0.0;
	  *minY = 0.0;
	  *maxX = rst->width;
	  *maxY = rst->height;
	  return RL2_OK;
      }
    *minX = rst->minX;
    *minY = rst->minY;
    *maxX = rst->maxX;
    *maxY = rst->maxY;
    return RL2_OK;
}

RL2_DECLARE gaiaGeomCollPtr
rl2_get_raster_bbox (rl2RasterPtr ptr)
{
/* return the Raster BBox [envelope] */
    gaiaGeomCollPtr geom;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return NULL;
    if (rst->Srid == RL2_GEOREFERENCING_NONE)
	return NULL;
    geom = gaiaAllocGeomColl ();
    geom->Srid = rst->Srid;
    pg = gaiaAddPolygonToGeomColl (geom, 5, 0);
    rng = pg->Exterior;
    gaiaSetPoint (rng->Coords, 0, rst->minX, rst->minY);
    gaiaSetPoint (rng->Coords, 1, rst->maxX, rst->minY);
    gaiaSetPoint (rng->Coords, 2, rst->maxX, rst->maxY);
    gaiaSetPoint (rng->Coords, 3, rst->minX, rst->maxY);
    gaiaSetPoint (rng->Coords, 4, rst->minX, rst->minY);
    return geom;
}

RL2_DECLARE int
rl2_get_raster_resolution (rl2RasterPtr ptr, double *hResolution,
			   double *vResolution)
{
/* return the Raster resolution */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return RL2_ERROR;
    if (rst->Srid == RL2_GEOREFERENCING_NONE)
	return RL2_ERROR;
    *hResolution = rst->hResolution;
    *vResolution = rst->vResolution;
    return RL2_OK;
}

RL2_DECLARE rl2PalettePtr
rl2_get_raster_palette (rl2RasterPtr ptr)
{
/* return a pointer to the Raster palette */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return NULL;
    return (rl2PalettePtr) (rst->Palette);
}

RL2_DECLARE int
rl2_raster_georeference_center (rl2RasterPtr ptr, int srid, double horz_res,
				double vert_res, double cx, double cy)
{
/* setting the Raster's georeferencing infos - Center Point */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    double hExt;
    double vExt;
    if (rst == NULL)
	return RL2_ERROR;
    rst->Srid = srid;
    rst->hResolution = horz_res;
    rst->vResolution = vert_res;
    hExt = horz_res * (double) (rst->width) / 2.0;
    vExt = vert_res * (double) (rst->height) / 2.0;
    rst->minX = cx - hExt;
    rst->minY = cy - vExt;
    rst->maxX = cx + hExt;
    rst->maxY = cy + vExt;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_georeference_upper_left (rl2RasterPtr ptr, int srid, double horz_res,
				    double vert_res, double x, double y)
{
/* setting the Raster's georeferencing infos - UpperLeft corner */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    double hExt;
    double vExt;
    if (rst == NULL)
	return RL2_ERROR;
    rst->Srid = srid;
    rst->hResolution = horz_res;
    rst->vResolution = vert_res;
    hExt = horz_res * (double) (rst->width);
    vExt = vert_res * (double) (rst->height);
    rst->minX = x;
    rst->minY = y - vExt;
    rst->maxX = x + hExt;
    rst->maxY = y;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_georeference_upper_right (rl2RasterPtr ptr, int srid,
				     double horz_res, double vert_res, double x,
				     double y)
{
/* setting the Raster's georeferencing infos - UpperRight corner */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    double hExt;
    double vExt;
    if (rst == NULL)
	return RL2_ERROR;
    rst->Srid = srid;
    rst->hResolution = horz_res;
    rst->vResolution = vert_res;
    hExt = horz_res * (double) (rst->width);
    vExt = vert_res * (double) (rst->height);
    rst->minX = x - hExt;
    rst->minY = y - vExt;
    rst->maxX = x;
    rst->maxY = y;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_georeference_lower_left (rl2RasterPtr ptr, int srid, double horz_res,
				    double vert_res, double x, double y)
{
/* setting the Raster's georeferencing infos - LowerLeft corner */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    double hExt;
    double vExt;
    if (rst == NULL)
	return RL2_ERROR;
    rst->Srid = srid;
    rst->hResolution = horz_res;
    rst->vResolution = vert_res;
    hExt = horz_res * (double) (rst->width);
    vExt = vert_res * (double) (rst->height);
    rst->minX = x;
    rst->minY = y;
    rst->maxX = x + hExt;
    rst->maxY = y + vExt;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_georeference_lower_right (rl2RasterPtr ptr, int srid,
				     double horz_res, double vert_res, double x,
				     double y)
{
/* setting the Raster's georeferencing infos - LowerRight corner */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    double hExt;
    double vExt;
    if (rst == NULL)
	return RL2_ERROR;
    rst->Srid = srid;
    rst->hResolution = horz_res;
    rst->vResolution = vert_res;
    hExt = horz_res * (double) (rst->width);
    vExt = vert_res * (double) (rst->height);
    rst->minX = x - hExt;
    rst->minY = y;
    rst->maxX = x;
    rst->maxY = y + vExt;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_georeference_frame (rl2RasterPtr ptr, int srid, double min_x,
			       double min_y, double max_x, double max_y)
{
/* setting the Raster's georeferencing infos - LowerRight corner */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    double hExt;
    double vExt;
    double horz_res;
    double vert_res;
    if (rst == NULL)
	return RL2_ERROR;
    if (max_x <= min_x)
	return RL2_ERROR;
    if (max_y <= min_y)
	return RL2_ERROR;

    rst->Srid = srid;
    rst->minX = min_x;
    rst->minY = min_y;
    rst->maxX = max_x;
    rst->maxY = max_y;
    hExt = max_x - min_x;
    vExt = max_y - min_y;
    horz_res = hExt / (double) (rst->width);
    vert_res = vExt / (double) (rst->height);
    rst->hResolution = horz_res;
    rst->vResolution = vert_res;
    hExt = horz_res * (double) (rst->width);
    vExt = vert_res * (double) (rst->height);
    return RL2_OK;
}

RL2_DECLARE rl2PalettePtr
rl2_create_palette (int num_entries)
{
/* allocating and initializing a Palette object */
    int i;
    rl2PrivPalettePtr plt = NULL;
    if (num_entries < 0 || num_entries > 256)
	return NULL;

    plt = malloc (sizeof (rl2PrivPalette));
    if (plt == NULL)
	return NULL;
    plt->nEntries = num_entries;
    if (num_entries == 0)
      {
	  plt->entries = NULL;
	  return (rl2PalettePtr) plt;
      }
    plt->entries = malloc (sizeof (rl2PrivPaletteEntry) * num_entries);
    if (plt->entries == NULL)
      {
	  free (plt);
	  return NULL;
      }
    for (i = 0; i < num_entries; i++)
      {
	  /* setting all entries as BLACK full opaque */
	  rl2PrivPaletteEntryPtr entry = plt->entries + i;
	  entry->red = 0;
	  entry->green = 0;
	  entry->blue = 0;
	  entry->alpha = 255;
      }
    return (rl2PalettePtr) plt;
}

RL2_DECLARE void
rl2_destroy_palette (rl2PalettePtr ptr)
{
/* memory cleanup - destroying a Palette object */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) ptr;
    if (plt == NULL)
	return;
    if (plt->entries != NULL)
	free (plt->entries);
    free (plt);
}

RL2_DECLARE rl2PalettePtr
rl2_clone_palette (rl2PalettePtr in)
{
/* cloning a Palette object */
    rl2PrivPalettePtr plt_in = (rl2PrivPalettePtr) in;
    rl2PalettePtr out;
    rl2PrivPalettePtr plt_out;
    int i;
    if (in == NULL)
	return NULL;
    out = rl2_create_palette (plt_in->nEntries);
    plt_out = (rl2PrivPalettePtr) out;
    for (i = 0; i < plt_out->nEntries; i++)
      {
	  rl2PrivPaletteEntryPtr entry_in = plt_in->entries + i;
	  rl2PrivPaletteEntryPtr entry_out = plt_out->entries + i;
	  entry_out->red = entry_in->red;
	  entry_out->green = entry_in->green;
	  entry_out->blue = entry_in->blue;
	  entry_out->alpha = entry_in->alpha;
      }
    return out;
}

static int
check_monochrome (int max, unsigned char *red, unsigned char *green,
		  unsigned char *blue)
{
/* testing for a MONOCHROME palette */
    if (max != 2)
	return 0;
    if (*(red + 0) != 255 || *(green + 0) != 255 || *(blue + 0) != 255)
	return 0;
    if (*(red + 1) != 0 || *(green + 1) != 0 || *(blue + 1) != 0)
	return 0;
    return 1;
}

static int
check_gray (int max, unsigned char *red, unsigned char *green,
	    unsigned char *blue)
{
/* testing for a GRAYSCALE palette */
    int i;
    if (max == 4)
      {
	  /* Gray4 */
	  if (*(red + 0) != 0 || *(green + 0) != 0 || *(blue + 0) != 0)
	      return 0;
	  if (*(red + 1) != 86 || *(green + 1) != 86 || *(blue + 1) != 86)
	      return 0;
	  if (*(red + 2) != 170 || *(green + 2) != 170 || *(blue + 2) != 170)
	      return 0;
	  if (*(red + 3) != 255 || *(green + 3) != 255 || *(blue + 3) != 255)
	      return 0;
	  return 1;
      }
    if (max == 16)
      {
	  /* Gray16 */
	  if (*(red + 0) != 0 || *(green + 0) != 0 || *(blue + 0) != 0)
	      return 0;
	  if (*(red + 1) != 17 || *(green + 1) != 17 || *(blue + 1) != 17)
	      return 0;
	  if (*(red + 2) != 34 || *(green + 2) != 34 || *(blue + 2) != 34)
	      return 0;
	  if (*(red + 3) != 51 || *(green + 3) != 51 || *(blue + 3) != 51)
	      return 0;
	  if (*(red + 4) != 68 || *(green + 4) != 68 || *(blue + 4) != 68)
	      return 0;
	  if (*(red + 5) != 85 || *(green + 5) != 85 || *(blue + 5) != 85)
	      return 0;
	  if (*(red + 6) != 102 || *(green + 6) != 102 || *(blue + 6) != 102)
	      return 0;
	  if (*(red + 7) != 119 || *(green + 7) != 119 || *(blue + 7) != 119)
	      return 0;
	  if (*(red + 8) != 137 || *(green + 8) != 137 || *(blue + 8) != 137)
	      return 0;
	  if (*(red + 9) != 154 || *(green + 9) != 154 || *(blue + 9) != 154)
	      return 0;
	  if (*(red + 10) != 171 || *(green + 10) != 171 || *(blue + 10) != 171)
	      return 0;
	  if (*(red + 11) != 188 || *(green + 11) != 188 || *(blue + 11) != 188)
	      return 0;
	  if (*(red + 12) != 205 || *(green + 12) != 205 || *(blue + 12) != 205)
	      return 0;
	  if (*(red + 13) != 222 || *(green + 13) != 222 || *(blue + 13) != 222)
	      return 0;
	  if (*(red + 14) != 239 || *(green + 14) != 239 || *(blue + 14) != 239)
	      return 0;
	  if (*(red + 15) != 255 || *(green + 15) != 255 || *(blue + 15) != 255)
	      return 0;
	  return 1;
      }
    if (max == 256)
      {
	  /* Gray256 */
	  for (i = 0; i < max; i++)
	    {
		if (*(red + i) != i || *(green + i) != i || *(blue + i) != i)
		    return 0;
	    }
	  return 1;
      }
    return 0;
}

RL2_DECLARE int
rl2_get_palette_type (rl2PalettePtr ptr, unsigned char *sample_type,
		      unsigned char *pixel_type)
{
/* retrieving the palette type */
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    unsigned char alpha[256];
    int max = 0;
    int index;
    int i;
    int already_defined;
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr palette = (rl2PrivPalettePtr) ptr;
    if (palette == NULL)
	return RL2_ERROR;

    for (index = 0; index < palette->nEntries; index++)
      {
	  /* normalizing the palette, so to avoid duplicate colors */
	  already_defined = 0;
	  entry = palette->entries + index;
	  for (i = 0; i < max; i++)
	    {
		if (entry->red == red[i] && entry->green == green[i]
		    && entry->blue == blue[i] && entry->alpha == alpha[i])
		  {
		      already_defined = 1;
		      break;
		  }
	    }
	  if (!already_defined)
	    {
		/* inserting a normalized color */
		red[max] = entry->red;
		green[max] = entry->green;
		blue[max] = entry->blue;
		alpha[max] = entry->alpha;
		max++;
	    }
      }
    if (max <= 2)
	*sample_type = RL2_SAMPLE_1_BIT;
    else if (max <= 4)
	*sample_type = RL2_SAMPLE_2_BIT;
    else if (max <= 16)
	*sample_type = RL2_SAMPLE_4_BIT;
    else
	*sample_type = RL2_SAMPLE_UINT8;
    *pixel_type = RL2_PIXEL_PALETTE;
    if (check_monochrome (max, red, green, blue))
	*pixel_type = RL2_PIXEL_MONOCHROME;
    else if (check_gray (max, red, green, blue))
	*pixel_type = RL2_PIXEL_GRAYSCALE;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_palette_color (rl2PalettePtr ptr, int index, unsigned char r,
		       unsigned char g, unsigned char b, unsigned char alpha)
{
/* setting a Palette entry */
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) ptr;
    if (plt == NULL)
	return RL2_ERROR;
    if (index < 0 || index >= plt->nEntries)
	return RL2_ERROR;
    entry = plt->entries + index;
    entry->red = r;
    entry->green = g;
    entry->blue = b;
    entry->alpha = alpha;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_palette_index (rl2PalettePtr ptr, unsigned char *index, unsigned char r,
		       unsigned char g, unsigned char b, unsigned char alpha)
{
/* finding the index corresponding to the given color (if any) */
    int i;
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) ptr;
    if (plt == NULL)
	return RL2_ERROR;
    for (i = 0; i < plt->nEntries; i++)
      {
	  entry = plt->entries + i;
	  if (entry->red == r && entry->green == g && entry->blue == b
	      && entry->alpha == alpha)
	    {
		*index = i;
		return RL2_OK;
	    }
      }
    return RL2_ERROR;
}

static int
parse_hex (unsigned char hi, unsigned char lo, unsigned char *x)
{
/* parsing an Hex Byte */
    unsigned char res;
    switch (hi)
      {
      case '0':
	  res = 0;
	  break;
      case '1':
	  res = 16;
	  break;
      case '2':
	  res = 2 * 16;
	  break;
      case '3':
	  res = 3 * 16;
	  break;
      case '4':
	  res = 4 * 16;
	  break;
      case '5':
	  res = 5 * 16;
	  break;
      case '6':
	  res = 6 * 16;
	  break;
      case '7':
	  res = 7 * 16;
	  break;
      case '8':
	  res = 8 * 16;
	  break;
      case '9':
	  res = 9 * 16;
	  break;
      case 'a':
      case 'A':
	  res = 10 * 16;
	  break;
      case 'b':
      case 'B':
	  res = 11 * 16;
	  break;
      case 'c':
      case 'C':
	  res = 12 * 16;
	  break;
      case 'd':
      case 'D':
	  res = 13 * 16;
	  break;
      case 'e':
      case 'E':
	  res = 14 * 16;
	  break;
      case 'f':
      case 'F':
	  res = 15 * 16;
	  break;
      default:
	  return RL2_ERROR;
      };
    switch (lo)
      {
      case '0':
	  break;
      case '1':
	  res += 1;
	  break;
      case '2':
	  res += 2;
	  break;
      case '3':
	  res += 3;
	  break;
      case '4':
	  res += 4;
	  break;
      case '5':
	  res += 5;
	  break;
      case '6':
	  res += 6;
	  break;
      case '7':
	  res += 7;
	  break;
      case '8':
	  res += 8;
	  break;
      case '9':
	  res += 9;
	  break;
      case 'a':
      case 'A':
	  res += 10;
	  break;
      case 'b':
      case 'B':
	  res += 11;
	  break;
      case 'c':
      case 'C':
	  res += 12;
	  break;
      case 'd':
      case 'D':
	  res += 13;
	  break;
      case 'e':
      case 'E':
	  res += 14;
	  break;
      case 'f':
      case 'F':
	  res += 15;
	  break;
      default:
	  return RL2_ERROR;
      };
    *x = res;
    return RL2_OK;
}

static int
parse_hex_rgb (const char *hex, unsigned char *r, unsigned char *g,
	       unsigned char *b)
{
/* parsing an Hex RGB */
    if (parse_hex (*(hex + 0), *(hex + 1), r) != RL2_OK)
	goto error;
    if (parse_hex (*(hex + 2), *(hex + 3), g) != RL2_OK)
	goto error;
    if (parse_hex (*(hex + 4), *(hex + 5), b) != RL2_OK)
	goto error;
    return RL2_OK;
  error:
    *r = 0;
    *g = 0;
    *b = 0;
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_set_palette_hexrgb (rl2PalettePtr ptr, int index, const char *hex)
{
/* setting a Palette entry */
    unsigned char r;
    unsigned char g;
    unsigned char b;
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) ptr;
    if (plt == NULL)
	return RL2_ERROR;
    if (index < 0 || index >= plt->nEntries)
	return RL2_ERROR;
    if (hex == NULL)
	return RL2_ERROR;
    if (strlen (hex) != 7)
	return RL2_ERROR;
    if (*hex != '#')
	return RL2_ERROR;
    if (parse_hex_rgb (hex + 1, &r, &g, &b) != RL2_OK)
	return RL2_ERROR;
    entry = plt->entries + index;
    entry->red = r;
    entry->green = g;
    entry->blue = b;
    entry->alpha = 255;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_palette_entries (rl2PalettePtr ptr, unsigned short *num_entries)
{
/* return the Palette entries count */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) ptr;
    if (plt == NULL)
	return RL2_ERROR;
    *num_entries = plt->nEntries;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_palette_colors (rl2PalettePtr ptr, unsigned short *num_entries,
			unsigned char **r, unsigned char **g, unsigned char **b,
			unsigned char **a)
{
/* return the Palette colors */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) ptr;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    unsigned char *alpha = NULL;
    int i;
    *num_entries = 0;
    *r = NULL;
    *g = NULL;
    *b = NULL;
    *a = NULL;
    if (plt == NULL)
	return RL2_ERROR;
    red = malloc (plt->nEntries);
    green = malloc (plt->nEntries);
    blue = malloc (plt->nEntries);
    alpha = malloc (plt->nEntries);
    if (red == NULL || green == NULL || blue == NULL || alpha == NULL)
      {
	  if (red != NULL)
	      free (red);
	  if (green != NULL)
	      free (green);
	  if (blue != NULL)
	      free (blue);
	  if (alpha != NULL)
	      free (alpha);
	  return RL2_ERROR;
      }
    for (i = 0; i < plt->nEntries; i++)
      {
	  rl2PrivPaletteEntryPtr entry = plt->entries + i;
	  *(red + i) = entry->red;
	  *(green + i) = entry->green;
	  *(blue + i) = entry->blue;
	  *(alpha + i) = entry->alpha;
      }
    *num_entries = plt->nEntries;
    *r = red;
    *g = green;
    *b = blue;
    *a = alpha;
    return RL2_OK;
}

RL2_DECLARE rl2PixelPtr
rl2_create_pixel (unsigned char sample_type, unsigned char pixel_type,
		  unsigned char num_samples)
{
/* allocating and initializing a Pixel object */
    int nBand;
    rl2PrivPixelPtr pxl = NULL;
    if (!is_valid_sample_type (sample_type))
	return NULL;
    if (!is_valid_pixel_type (pixel_type))
	return NULL;
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_DATAGRID:
	  if (num_samples != 1)
	      return NULL;
	  break;
      case RL2_PIXEL_RGB:
	  if (num_samples != 3)
	      return NULL;
	  break;
      case RL2_PIXEL_MULTIBAND:
	  if (num_samples < 2)
	      return NULL;
	  break;
      };

    pxl = malloc (sizeof (rl2PrivPixel));
    if (pxl == NULL)
	return NULL;
    pxl->sampleType = sample_type;
    pxl->pixelType = pixel_type;
    pxl->nBands = num_samples;
    pxl->isTransparent = 0;
    pxl->Samples = malloc (sizeof (rl2PrivSample) * num_samples);
    if (pxl->Samples == NULL)
      {
	  free (pxl);
	  return NULL;
      }
    for (nBand = 0; nBand < num_samples; nBand++)
      {
	  /* initializing all samples as ZERO */
	  rl2PrivSamplePtr sample = pxl->Samples + nBand;
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		sample->uint8 = 0;
		break;
	    case RL2_SAMPLE_INT8:
		sample->int8 = 0;
		break;
	    case RL2_SAMPLE_INT16:
		sample->int16 = 0;
		break;
	    case RL2_SAMPLE_UINT16:
		sample->uint16 = 0;
		break;
	    case RL2_SAMPLE_INT32:
		sample->int32 = 0;
		break;
	    case RL2_SAMPLE_UINT32:
		sample->uint32 = 0;
		break;
	    case RL2_SAMPLE_FLOAT:
		sample->float32 = 0.0;
		break;
	    case RL2_SAMPLE_DOUBLE:
		sample->float64 = 0.0;
		break;
	    }
      }
    return (rl2PixelPtr) pxl;
}

RL2_DECLARE void
rl2_destroy_pixel (rl2PixelPtr ptr)
{
/* memory cleanup - destroying a Pixel object */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return;
    if (pxl->Samples != NULL)
	free (pxl->Samples);
    free (pxl);
}

RL2_DECLARE int
rl2_compare_pixels (rl2PixelPtr pixel1, rl2PixelPtr pixel2)
{
/* comparing if two pixels are the same */
    int band;
    rl2PrivPixelPtr pxl1 = (rl2PrivPixelPtr) pixel1;
    rl2PrivPixelPtr pxl2 = (rl2PrivPixelPtr) pixel2;
    if (pxl1 == NULL || pxl2 == NULL)
	return RL2_ERROR;
    if (pxl1->sampleType != pxl2->sampleType)
	return RL2_ERROR;
    if (pxl1->pixelType != pxl2->pixelType)
	return RL2_ERROR;
    if (pxl1->nBands != pxl2->nBands)
	return RL2_ERROR;
    for (band = 0; band < pxl1->nBands; band++)
      {
	  rl2PrivSamplePtr sample1 = pxl1->Samples + band;
	  rl2PrivSamplePtr sample2 = pxl2->Samples + band;
	  switch (pxl1->sampleType)
	    {
	    case RL2_SAMPLE_INT8:
		if (sample1->int8 != sample2->int8)
		    return RL2_FALSE;
		break;
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		if (sample1->uint8 != sample2->uint8)
		    return RL2_FALSE;
		break;
	    case RL2_SAMPLE_INT16:
		if (sample1->int16 != sample2->int16)
		    return RL2_FALSE;
		break;
	    case RL2_SAMPLE_UINT16:
		if (sample1->uint16 != sample2->uint16)
		    return RL2_FALSE;
		break;
	    case RL2_SAMPLE_INT32:
		if (sample1->int32 != sample2->int32)
		    return RL2_FALSE;
		break;
	    case RL2_SAMPLE_UINT32:
		if (sample1->uint32 != sample2->uint32)
		    return RL2_FALSE;
		break;
	    case RL2_SAMPLE_FLOAT:
		if (sample1->float32 != sample2->float32)
		    return RL2_FALSE;
		break;
	    case RL2_SAMPLE_DOUBLE:
		if (sample1->float64 != sample2->float64)
		    return RL2_FALSE;
		break;
	    };
      }
    if (pxl1->isTransparent != pxl2->isTransparent)
	return RL2_FALSE;
    return RL2_TRUE;
}


RL2_DECLARE int
rl2_get_pixel_type (rl2PixelPtr ptr,
		    unsigned char *sample_type,
		    unsigned char *pixel_type, unsigned char *num_bands)
{
/* return the Pixel pixel type */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    *sample_type = pxl->sampleType;
    *pixel_type = pxl->pixelType;
    *num_bands = pxl->nBands;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_1bit (rl2PixelPtr ptr, unsigned char *sample)
{
/* get a Pixel/Sample value - 1-BIT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_1_BIT)
	return RL2_ERROR;
    *sample = pxl->Samples->uint8;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_1bit (rl2PixelPtr ptr, unsigned char sample)
{
/* set a Pixel/Sample value - 1-BIT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_1_BIT)
	return RL2_ERROR;
    if (sample > 1)
	return RL2_ERROR;
    pxl->Samples->uint8 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_2bit (rl2PixelPtr ptr, unsigned char *sample)
{
/* get a Pixel/Sample value - 2-BIT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_2_BIT)
	return RL2_ERROR;
    *sample = pxl->Samples->uint8;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_2bit (rl2PixelPtr ptr, unsigned char sample)
{
/* set a Pixel/Sample value - 2-BIT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_2_BIT)
	return RL2_ERROR;
    if (sample > 3)
	return RL2_ERROR;
    pxl->Samples->uint8 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_4bit (rl2PixelPtr ptr, unsigned char *sample)
{
/* get a Pixel/Sample value - 4-BIT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_4_BIT)
	return RL2_ERROR;
    *sample = pxl->Samples->uint8;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_4bit (rl2PixelPtr ptr, unsigned char sample)
{
/* set a Pixel/Sample value - 4-BIT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_4_BIT)
	return RL2_ERROR;
    if (sample > 15)
	return RL2_ERROR;
    pxl->Samples->uint8 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_int8 (rl2PixelPtr ptr, char *sample)
{
/* get a Pixel/Sample value - INT8 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_INT8)
	return RL2_ERROR;
    *sample = pxl->Samples->int8;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_int8 (rl2PixelPtr ptr, char sample)
{
/* set a Pixel/Sample value - INT8 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_INT8)
	return RL2_ERROR;
    pxl->Samples->int8 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_uint8 (rl2PixelPtr ptr, int band, unsigned char *sample)
{
/* get a Pixel/Sample value - UINT8 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_UINT8)
	return RL2_ERROR;
    if (band >= 0 && band < pxl->nBands)
      {
	  rl2PrivSamplePtr smp = pxl->Samples + band;
	  *sample = smp->uint8;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_set_pixel_sample_uint8 (rl2PixelPtr ptr, int band, unsigned char sample)
{
/* set a Pixel/Sample value - UINT8 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_UINT8)
	return RL2_ERROR;
    if (band >= 0 && band < pxl->nBands)
      {
	  rl2PrivSamplePtr smp = pxl->Samples + band;
	  smp->uint8 = sample;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_pixel_sample_int16 (rl2PixelPtr ptr, short *sample)
{
/* get a Pixel/Sample value - INT16 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_INT16)
	return RL2_ERROR;
    *sample = pxl->Samples->int16;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_int16 (rl2PixelPtr ptr, short sample)
{
/* set a Pixel/Sample value - INT16 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_INT16)
	return RL2_ERROR;
    pxl->Samples->int16 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_uint16 (rl2PixelPtr ptr, int band, unsigned short *sample)
{
/* get a Pixel/Sample value - UINT16 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_UINT16)
	return RL2_ERROR;
    if (band >= 0 && band < pxl->nBands)
      {
	  rl2PrivSamplePtr smp = pxl->Samples + band;
	  *sample = smp->uint16;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_set_pixel_sample_uint16 (rl2PixelPtr ptr, int band, unsigned short sample)
{
/* set a Pixel/Sample value - UINT16 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_UINT16)
	return RL2_ERROR;
    if (band >= 0 && band < pxl->nBands)
      {
	  rl2PrivSamplePtr smp = pxl->Samples + band;
	  smp->uint16 = sample;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_pixel_sample_int32 (rl2PixelPtr ptr, int *sample)
{
/* get a Pixel/Sample value - INT32 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_INT32)
	return RL2_ERROR;
    *sample = pxl->Samples->int32;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_int32 (rl2PixelPtr ptr, int sample)
{
/* set a Pixel/Sample value - INT32 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_INT32)
	return RL2_ERROR;
    pxl->Samples->int32 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_uint32 (rl2PixelPtr ptr, unsigned int *sample)
{
/* get a Pixel/Sample value - UINT32 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_UINT32)
	return RL2_ERROR;
    *sample = pxl->Samples->uint32;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_uint32 (rl2PixelPtr ptr, unsigned int sample)
{
/* set a Pixel/Sample value - UINT32 */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_UINT32)
	return RL2_ERROR;
    pxl->Samples->uint32 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_float (rl2PixelPtr ptr, float *sample)
{
/* get a Pixel/Sample value - FLOAT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_FLOAT)
	return RL2_ERROR;
    *sample = pxl->Samples->float32;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_float (rl2PixelPtr ptr, float sample)
{
/* set a Pixel/Sample value - FLOAT */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_FLOAT)
	return RL2_ERROR;
    pxl->Samples->float32 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_pixel_sample_double (rl2PixelPtr ptr, double *sample)
{
/* get a Pixel/Sample value - DOUBLE */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_DOUBLE)
	return RL2_ERROR;
    *sample = pxl->Samples->float64;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_sample_double (rl2PixelPtr ptr, double sample)
{
/* set a Pixel/Sample value - DOUBLE */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->sampleType != RL2_SAMPLE_DOUBLE)
	return RL2_ERROR;
    pxl->Samples->float64 = sample;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_pixel_transparent (rl2PixelPtr ptr, int *is_transparent)
{
/* tests for a transparent pixel */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->isTransparent)
	*is_transparent = RL2_TRUE;
    else
	*is_transparent = RL2_FALSE;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_pixel_opaque (rl2PixelPtr ptr, int *is_opaque)
{
/* tests for an opaque pixel */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    if (pxl->isTransparent)
	*is_opaque = RL2_FALSE;
    else
	*is_opaque = RL2_TRUE;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_transparent (rl2PixelPtr ptr)
{
/* marks a pixel as transparent */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    pxl->isTransparent = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_pixel_opaque (rl2PixelPtr ptr)
{
/* marks a pixel as opaque */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) ptr;
    if (pxl == NULL)
	return RL2_ERROR;
    pxl->isTransparent = 0;
    return RL2_OK;
}

RL2_DECLARE rl2PixelPtr
rl2_create_raster_pixel (rl2RasterPtr ptr)
{
/* creating a Pixel matching the given Raster */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst == NULL)
	return NULL;
    return rl2_create_pixel (rst->sampleType, rst->pixelType, rst->nBands);
}

RL2_DECLARE int
rl2_get_raster_pixel (rl2RasterPtr ptr, rl2PixelPtr pixel, unsigned short row,
		      unsigned short col)
{
/* fetching a Pixel from a Raster */
    int nBand;
    rl2PrivSamplePtr sample;
    char *p_int8;
    unsigned char *p_uint8;
    short *p_int16;
    unsigned short *p_uint16;
    int *p_int32;
    unsigned int *p_uint32;
    float *p_float32;
    double *p_float64;
    unsigned char *mask;
    unsigned char *p_mask;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) pixel;
    if (rst == NULL)
	return RL2_ERROR;
    if (pxl == NULL)
	return RL2_ERROR;

    if (pxl->sampleType != rst->sampleType || pxl->pixelType != rst->pixelType
	|| pxl->nBands != rst->nBands)
	return RL2_ERROR;
    if (row >= rst->height || col >= rst->width)
	return RL2_ERROR;

    for (nBand = 0; nBand < pxl->nBands; nBand++)
      {
	  sample = pxl->Samples + nBand;
	  switch (pxl->sampleType)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		p_uint8 = rst->rasterBuffer;
		p_uint8 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->uint8 = *p_uint8;
		break;
	    case RL2_SAMPLE_INT8:
		p_int8 = (char *) (rst->rasterBuffer);
		p_int8 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->int8 = *p_int8;
		break;
	    case RL2_SAMPLE_INT16:
		p_int16 = (short *) (rst->rasterBuffer);
		p_int16 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->int16 = *p_int16;
		break;
	    case RL2_SAMPLE_UINT16:
		p_uint16 = (unsigned short *) (rst->rasterBuffer);
		p_uint16 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->uint16 = *p_uint16;
		break;
	    case RL2_SAMPLE_INT32:
		p_int32 = (int *) (rst->rasterBuffer);
		p_int32 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->int32 = *p_int32;
		break;
	    case RL2_SAMPLE_UINT32:
		p_uint32 = (unsigned int *) (rst->rasterBuffer);
		p_uint32 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->uint32 = *p_uint32;
		break;
	    case RL2_SAMPLE_FLOAT:
		p_float32 = (float *) (rst->rasterBuffer);
		p_float32 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->float32 = *p_float32;
		break;
	    case RL2_SAMPLE_DOUBLE:
		p_float64 = (double *) (rst->rasterBuffer);
		p_float64 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		sample->float64 = *p_float64;
		break;
	    };
      }

/* transparency */
    pxl->isTransparent = 0;
    mask = rst->maskBuffer;
    if (mask != NULL)
      {
	  /* evaluating transparency mask */
	  p_mask = mask + (row * rst->width) + col;
	  if (*p_mask == 0)
	      pxl->isTransparent = 1;
      }
    if (rst->noData != NULL)
      {
	  /* evaluating transparent color */
	  if (rl2_compare_pixels (pixel, (rl2PixelPtr) (rst->noData)) ==
	      RL2_TRUE)
	      pxl->isTransparent = 1;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_raster_pixel (rl2RasterPtr ptr, rl2PixelPtr pixel, unsigned short row,
		      unsigned short col)
{
/* changing a Pixel into a Raster */
    int nBand;
    rl2PrivSamplePtr sample;
    char *p_int8;
    unsigned char *p_uint8;
    short *p_int16;
    unsigned short *p_uint16;
    int *p_int32;
    unsigned int *p_uint32;
    float *p_float32;
    double *p_float64;
    unsigned char *mask;
    unsigned char *p_mask;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) pixel;
    if (rst == NULL)
	return RL2_ERROR;
    if (pxl == NULL)
	return RL2_ERROR;

    if (pxl->sampleType != rst->sampleType || pxl->pixelType != rst->pixelType
	|| pxl->nBands != rst->nBands)
	return RL2_ERROR;
    if (row >= rst->height || col >= rst->width)
	return RL2_ERROR;
    if (pxl->pixelType == RL2_PIXEL_PALETTE)
      {
	  /* blocking any attempt to insert out-palette values */
	  rl2PrivPalettePtr plt = (rl2PrivPalettePtr) rst->Palette;
	  if (pxl->Samples->uint8 >= plt->nEntries)
	      return RL2_ERROR;
      }

    for (nBand = 0; nBand < pxl->nBands; nBand++)
      {
	  sample = pxl->Samples + nBand;
	  switch (pxl->sampleType)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		p_uint8 = rst->rasterBuffer;
		p_uint8 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_uint8 = sample->uint8;
		break;
	    case RL2_SAMPLE_INT8:
		p_int8 = (char *) (rst->rasterBuffer);
		p_int8 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_int8 = sample->int8;
		break;
	    case RL2_SAMPLE_INT16:
		p_int16 = (short *) (rst->rasterBuffer);
		p_int16 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_int16 = sample->int16;
		break;
	    case RL2_SAMPLE_UINT16:
		p_uint16 = (unsigned short *) (rst->rasterBuffer);
		p_uint16 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_uint16 = sample->uint16;
		break;
	    case RL2_SAMPLE_INT32:
		p_int32 = (int *) (rst->rasterBuffer);
		p_int32 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_int32 = sample->int32;
		break;
	    case RL2_SAMPLE_UINT32:
		p_uint32 = (unsigned int *) (rst->rasterBuffer);
		p_uint32 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_uint32 = sample->uint32;
		break;
	    case RL2_SAMPLE_FLOAT:
		p_float32 = (float *) (rst->rasterBuffer);
		p_float32 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_float32 = sample->float32;
		break;
	    case RL2_SAMPLE_DOUBLE:
		p_float64 = (double *) (rst->rasterBuffer);
		p_float64 +=
		    (row * rst->width * pxl->nBands) + (col * pxl->nBands) +
		    nBand;
		*p_float64 = sample->float64;
		break;
	    };
      }

    mask = rst->maskBuffer;
    if (mask != NULL)
      {
	  /* updating the transparency mask */
	  p_mask = mask + (row * rst->width) + col;
	  if (pxl->isTransparent)
	      *p_mask = 0;
	  else
	      *p_mask = 1;
      }
    return RL2_OK;
}

RL2_DECLARE rl2RasterStatisticsPtr
rl2_create_raster_statistics (unsigned char sample_type,
			      unsigned char num_bands)
{
/* allocating and initializing a Raster Statistics object */
    int i;
    int j;
    int nHistogram;
    rl2PrivRasterStatisticsPtr stats = NULL;
    if (num_bands == 0)
	return NULL;
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
	  nHistogram = 2;
	  break;
      case RL2_SAMPLE_2_BIT:
	  nHistogram = 4;
	  break;
      case RL2_SAMPLE_4_BIT:
	  nHistogram = 16;
	  break;
      default:
	  nHistogram = 256;
	  break;
      };

    stats = malloc (sizeof (rl2PrivRasterStatistics));
    if (stats == NULL)
	return NULL;
    stats->no_data = 0.0;
    stats->count = 0.0;
    stats->sampleType = sample_type;
    stats->nBands = num_bands;
    stats->band_stats = malloc (sizeof (rl2PrivBandStatistics) * num_bands);
    if (stats->band_stats == NULL)
      {
	  free (stats);
	  return NULL;
      }
    for (i = 0; i < num_bands; i++)
      {
	  /* initializing the Bands Statistics */
	  rl2PrivBandStatisticsPtr band = stats->band_stats + i;
	  band->min = DBL_MAX;
	  band->max = 0.0 - DBL_MAX;
	  band->mean = 0.0;
	  band->sum_sq_diff = 0.0;
	  band->nHistogram = nHistogram;
	  band->histogram = malloc (sizeof (double) * nHistogram);
	  for (j = 0; j < nHistogram; j++)
	      band->histogram[j] = 0.0;
	  band->first = NULL;
	  band->last = NULL;
      }
    return (rl2RasterStatisticsPtr) stats;
}

static void
free_band_stats (rl2PrivBandStatisticsPtr band)
{
/* memory cleanup - destroying a Raster Band Statistics object */
    rl2PoolVariancePtr pV;
    rl2PoolVariancePtr pVn;
    if (band == NULL)
	return;
    if (band->histogram != NULL)
	free (band->histogram);
    pV = band->first;
    while (pV != NULL)
      {
	  pVn = pV->next;
	  free (pV);
	  pV = pVn;
      }
}

RL2_DECLARE void
rl2_destroy_raster_statistics (rl2RasterStatisticsPtr stats)
{
/* memory cleanup - destroying a Raster Statistics object */
    int nb;
    rl2PrivRasterStatisticsPtr st = (rl2PrivRasterStatisticsPtr) stats;
    if (st == NULL)
	return;
    for (nb = 0; nb < st->nBands; nb++)
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + nb;
	  free_band_stats (band);
      }
    if (st->band_stats != NULL)
	free (st->band_stats);
    free (st);
}

RL2_DECLARE int
rl2_get_raster_statistics_summary (rl2RasterStatisticsPtr stats,
				   double *no_data, double *count,
				   unsigned char *sample_type,
				   unsigned char *num_bands)
{
/* returning overall statistics values */
    rl2PrivRasterStatisticsPtr st = (rl2PrivRasterStatisticsPtr) stats;
    if (st == NULL)
	return RL2_ERROR;
    *no_data = st->no_data;
    *count = st->count;
    *sample_type = st->sampleType;
    *num_bands = st->nBands;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_band_statistics (rl2RasterStatisticsPtr stats, unsigned char band,
			 double *min, double *max, double *mean,
			 double *variance, double *standard_deviation)
{
/* returning Band statistics values */
    rl2PrivBandStatisticsPtr st_band;
    rl2PrivRasterStatisticsPtr st = (rl2PrivRasterStatisticsPtr) stats;
    if (st == NULL)
	return RL2_ERROR;
    if (band >= st->nBands)
	return RL2_ERROR;

    st_band = st->band_stats + band;
    *min = st_band->min;
    *max = st_band->max;
    *mean = st_band->mean;
    if (st_band->first != NULL)
      {
	  double count = 0.0;
	  double sum_var = 0.0;
	  double sum_count = 0.0;
	  rl2PoolVariancePtr pV = st_band->first;
	  while (pV != NULL)
	    {
		count += 1.0;
		sum_var += (pV->count - 1.0) * pV->variance;
		sum_count += pV->count;
		pV = pV->next;
	    }
	  *variance = sum_var / (sum_count - count);
      }
    else
	*variance = st_band->sum_sq_diff / (st->count - 1.0);
    *standard_deviation = sqrt (*variance);
    return RL2_OK;
}
