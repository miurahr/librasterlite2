/*

 rl2ascii -- ASCII Grids related functions

 version 0.1, 2013 December 26

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

#include "rasterlite2/sqlite.h"

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

static int
parse_ncols (const char *str, unsigned short *width)
{
/* attempting to parse the NCOLS item */
    if (strncmp (str, "ncols ", 6) == 0)
      {
	  *width = atoi (str + 6);
	  return 1;
      }
    return 0;
}

static int
parse_nrows (const char *str, unsigned short *height)
{
/* attempting to parse the NROWS item */
    if (strncmp (str, "nrows ", 6) == 0)
      {
	  *height = atoi (str + 6);
	  return 1;
      }
    return 0;
}

static int
parse_xllcorner (const char *str, double *minx)
{
/* attempting to parse the XLLCORNER item */
    if (strncmp (str, "xllcorner ", 10) == 0)
      {
	  *minx = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_yllcorner (const char *str, double *miny)
{
/* attempting to parse the YLLCORNER item */
    if (strncmp (str, "yllcorner ", 10) == 0)
      {
	  *miny = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_cellsize (const char *str, double *xres)
{
/* attempting to parse the CELLSIZE item */
    if (strncmp (str, "cellsize ", 9) == 0)
      {
	  *xres = atof (str + 9);
	  return 1;
      }
    return 0;
}

static int
parse_nodata (const char *str, double *no_data)
{
/* attempting to parse the NODATA_value item */
    if (strncmp (str, "NODATA_value ", 13) == 0)
      {
	  *no_data = atof (str + 13);
	  return 1;
      }
    return 0;
}

static int
get_ascii_header (FILE * in, unsigned short *width, unsigned short *height,
		  double *minx, double *miny, double *maxx, double *maxy,
		  double *xres, double *yres, double *no_data)
{
/* attempting to parse the ASCII Header */
    char buf[1024];
    char *p_out = buf;
    int line_no = 0;
    int c;

    while ((c = getc (in)) != EOF)
      {
	  if (c == '\r')
	      continue;
	  if (c == '\n')
	    {
		*p_out = '\0';
		switch (line_no)
		  {
		  case 0:
		      if (!parse_ncols (buf, width))
			  goto error;
		      break;
		  case 1:
		      if (!parse_nrows (buf, height))
			  goto error;
		      break;
		  case 2:
		      if (!parse_xllcorner (buf, minx))
			  goto error;
		      break;
		  case 3:
		      if (!parse_yllcorner (buf, miny))
			  goto error;
		      break;
		  case 4:
		      if (!parse_cellsize (buf, xres))
			  goto error;
		      break;
		  case 5:
		      if (!parse_nodata (buf, no_data))
			  goto error;
		      break;
		  };
		line_no++;
		if (line_no == 6)
		    break;
		p_out = buf;
		continue;
	    }
	  if ((p_out - buf) >= 1024)
	      goto error;
	  *p_out++ = c;
      }

    *maxx = *minx + ((double) (*width) * *xres);
    *yres = *xres;
    *maxy = *miny + ((double) (*height) * *yres);
    return 1;

  error:
    *width = 0;
    *height = 0;
    *minx = DBL_MAX;
    *miny = DBL_MAX;
    *maxx = 0.0 - DBL_MAX;
    *maxy = 0.0 - DBL_MAX;
    *xres = 0.0;
    *yres = 0.0;
    *no_data = DBL_MAX;
    return 0;
}

static rl2PrivAsciiOriginPtr
alloc_ascii_origin (const char *path, int srid, unsigned char sample_type,
		    unsigned short width, unsigned short height, double minx,
		    double miny, double maxx, double maxy, double xres,
		    double yres, double no_data)
{
/* allocating and initializing an ASCII Grid origin */
    int len;
    rl2PrivAsciiOriginPtr ascii = malloc (sizeof (rl2PrivAsciiOrigin));
    if (ascii == NULL)
	return NULL;
    len = strlen (path);
    ascii->path = malloc (len + 1);
    strcpy (ascii->path, path);
    ascii->tmp = NULL;
    ascii->width = width;
    ascii->height = height;
    ascii->Srid = srid;
    ascii->hResolution = xres;
    ascii->vResolution = yres;
    ascii->minX = minx;
    ascii->minY = miny;
    ascii->maxX = maxx;
    ascii->maxY = maxy;
    ascii->sample_type = sample_type;
    ascii->noData = no_data;
    return ascii;
}

RL2_DECLARE rl2AsciiOriginPtr
rl2_create_ascii_origin (const char *path, int srid, unsigned char sample_type)
{
/* creating an ASCII Grid Origin */
    FILE *in;
    unsigned short width;
    unsigned short height;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double xres;
    double yres;
    double no_data;
    char buf[1024];
    char *p_out = buf;
    int line_no = 0;
    int col_no = 0;
    int new_line = 1;
    int c;
    rl2PrivAsciiOriginPtr ascii = NULL;
    void *scanline = NULL;
    char *p_int8;
    unsigned char *p_uint8;
    short *p_int16;
    unsigned short *p_uint16;
    int *p_int32;
    unsigned int *p_uint32;
    float *p_float;
    double *p_double;
    int sz;

    if (path == NULL)
	return NULL;
    if (srid <= 0)
	return NULL;
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
	  return NULL;
      };

    in = fopen (path, "r");
    if (in == NULL)
      {
	  fprintf (stderr, "ASCII Origin: Unable to open %s\n", path);
	  return NULL;
      }
    if (!get_ascii_header
	(in, &width, &height, &minx, &miny, &maxx, &maxy, &xres, &yres,
	 &no_data))
      {
	  fprintf (stderr, "ASCII Origin: invalid Header found on %s\n", path);
	  goto error;
      }

    ascii =
	alloc_ascii_origin (path, srid, sample_type, width, height, minx, miny,
			    maxx, maxy, xres, yres, no_data);
    if (ascii == NULL)
	goto error;

    *buf = '\0';
    col_no = width;
/* creating the helper Temporary File */
    ascii->tmp = tmpfile ();
    if (ascii->tmp == NULL)
	goto error;
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
	  sz = ascii->width;
	  break;
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  sz = ascii->width * 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  sz = ascii->width * 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sz = ascii->width * 8;
	  break;
      };
    scanline = malloc (sz);
    if (scanline == NULL)
	goto error;

    while ((c = getc (in)) != EOF)
      {
	  if (c == '\r')
	      continue;
	  if (c == ' ' || c == '\n')
	    {
		*p_out = '\0';
		if (*buf != '\0')
		  {
		      char int8_value;
		      unsigned char uint8_value;
		      short int16_value;
		      unsigned short uint16_value;
		      int int32_value;
		      unsigned int uint32_value;
		      float flt_value;
		      double dbl_value = atof (buf);
		      if (new_line)
			{
			    if (col_no != width)
				goto error;
			    line_no++;
			    new_line = 0;
			    col_no = 0;
			    switch (sample_type)
			      {
			      case RL2_SAMPLE_INT8:
				  p_int8 = scanline;
				  break;
			      case RL2_SAMPLE_UINT8:
				  p_uint8 = scanline;
				  break;
			      case RL2_SAMPLE_INT16:
				  p_int16 = scanline;
				  break;
			      case RL2_SAMPLE_UINT16:
				  p_uint16 = scanline;
				  break;
			      case RL2_SAMPLE_INT32:
				  p_int32 = scanline;
				  break;
			      case RL2_SAMPLE_UINT32:
				  p_uint32 = scanline;
				  break;
			      case RL2_SAMPLE_FLOAT:
				  p_float = scanline;
				  break;
			      case RL2_SAMPLE_DOUBLE:
				  p_double = scanline;
				  break;
			      };
			}
		      switch (sample_type)
			{
			case RL2_SAMPLE_INT8:
			    int8_value = (char) dbl_value;
			    if (fabs (dbl_value - int8_value) > 1.0)
				goto error;
			    *p_int8++ = uint8_value;
			    break;
			case RL2_SAMPLE_UINT8:
			    uint8_value = (unsigned char) dbl_value;
			    if (fabs (dbl_value - uint8_value) > 1.0)
				goto error;
			    *p_uint8++ = uint8_value;
			    break;
			case RL2_SAMPLE_INT16:
			    int16_value = (short) dbl_value;
			    if (fabs (dbl_value - int16_value) > 1.0)
				goto error;
			    *p_int16++ = int16_value;
			    break;
			case RL2_SAMPLE_UINT16:
			    uint16_value = (unsigned short) dbl_value;
			    if (fabs (dbl_value - uint16_value) > 1.0)
				goto error;
			    *p_uint16++ = uint16_value;
			    break;
			case RL2_SAMPLE_INT32:
			    int32_value = (int) dbl_value;
			    if (fabs (dbl_value - int32_value) > 1.0)
				goto error;
			    *p_int32++ = int32_value;
			    break;
			case RL2_SAMPLE_UINT32:
			    uint32_value = (unsigned int) dbl_value;
			    if (fabs (dbl_value - uint32_value) > 1.0)
				goto error;
			    *p_uint32++ = uint32_value;
			    break;
			case RL2_SAMPLE_FLOAT:
			    flt_value = (float) dbl_value;
			    if (fabs (dbl_value - flt_value) > 1.0)
				goto error;
			    *p_float++ = flt_value;
			    break;
			case RL2_SAMPLE_DOUBLE:
			    *p_double++ = dbl_value;
			    break;
			};
		      col_no++;
		      if (col_no == ascii->width)
			  fwrite (scanline, sz, 1, ascii->tmp);
		  }
		p_out = buf;
		if (c == '\n')
		    new_line = 1;
		continue;
	    }

	  if ((p_out - buf) >= 1024)
	      goto error;
	  *p_out++ = c;
      }
    if (line_no != height)
	goto error;

    fclose (in);
    free (scanline);
    return (rl2AsciiOriginPtr) ascii;

  error:
    if (scanline != NULL)
	free (scanline);
    if (ascii != NULL)
	rl2_destroy_ascii_origin ((rl2AsciiOriginPtr) ascii);
    if (in != NULL)
	fclose (in);
    return NULL;
}

RL2_DECLARE void
rl2_destroy_ascii_origin (rl2AsciiOriginPtr ascii)
{
/* memory cleanup - destroying an ASCII Grid object */
    rl2PrivAsciiOriginPtr org = (rl2PrivAsciiOriginPtr) ascii;
    if (org == NULL)
	return;
    if (org->path != NULL)
	free (org->path);
    if (org->tmp != NULL)
	fclose (org->tmp);
    free (org);
}

RL2_DECLARE const char *
rl2_get_ascii_origin_path (rl2AsciiOriginPtr ascii)
{
/* retrieving the input path from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return NULL;

    return origin->path;
}

RL2_DECLARE int
rl2_get_ascii_origin_size (rl2AsciiOriginPtr ascii, unsigned short *width,
			   unsigned short *height)
{
/* retrieving Width and Height from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *width = origin->width;
    *height = origin->height;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_origin_srid (rl2AsciiOriginPtr ascii, int *srid)
{
/* retrieving the SRID from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *srid = origin->Srid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_origin_extent (rl2AsciiOriginPtr ascii, double *minX,
			     double *minY, double *maxX, double *maxY)
{
/* retrieving the Extent from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *minX = origin->minX;
    *minY = origin->minY;
    *maxX = origin->maxX;
    *maxY = origin->maxY;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_origin_resolution (rl2AsciiOriginPtr ascii, double *hResolution,
				 double *vResolution)
{
/* retrieving the Pixel Resolution from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *hResolution = origin->hResolution;
    *vResolution = origin->vResolution;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_origin_type (rl2AsciiOriginPtr ascii, unsigned char *sample_type,
			   unsigned char *pixel_type, unsigned char *num_bands)
{
/* retrieving the sample/pixel type from an ASCII Grid origin */
    int ok = 0;
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *sample_type = origin->sample_type;
    *pixel_type = RL2_PIXEL_DATAGRID;
    *num_bands = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_eval_ascii_origin_compatibility (rl2CoveragePtr cvg,
				     rl2AsciiOriginPtr ascii)
{
/* testing if a Coverage and an ASCII Grid origin are mutually compatible */
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    double hResolution;
    double vResolution;
    double confidence;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;

    if (coverage == NULL || ascii == NULL)
	return RL2_ERROR;
    if (rl2_get_ascii_origin_type (ascii, &sample_type, &pixel_type, &num_bands)
	!= RL2_OK)
	return RL2_ERROR;

    if (coverage->sampleType != sample_type)
	return RL2_FALSE;
    if (coverage->pixelType != pixel_type)
	return RL2_FALSE;
    if (coverage->nBands != num_bands)
	return RL2_FALSE;

/* checking for resolution compatibility */
    if (rl2_get_ascii_origin_srid (ascii, &srid) != RL2_OK)
	return RL2_FALSE;
    if (coverage->Srid != srid)
	return RL2_FALSE;
    if (rl2_get_ascii_origin_resolution (ascii, &hResolution, &vResolution) !=
	RL2_OK)
	return RL2_FALSE;
    confidence = coverage->hResolution / 100.0;
    if (hResolution < (coverage->hResolution - confidence)
	|| hResolution > (coverage->hResolution + confidence))
	return RL2_FALSE;
    confidence = coverage->vResolution / 100.0;
    if (vResolution < (coverage->vResolution - confidence)
	|| vResolution > (coverage->vResolution + confidence))
	return RL2_FALSE;
    return RL2_TRUE;
}

static int
read_ascii_int8 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		 unsigned short height, unsigned int startRow,
		 unsigned int startCol, char *pixels)
{
/* reading from the Temporary helper file - INT8 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset = (row * origin->width) + startCol;
	  char *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		char int8;
		if (fread (&int8, sizeof (char), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = int8;
	    }
      }
    return 1;
}

static int
read_ascii_uint8 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, unsigned char *pixels)
{
/* reading from the Temporary helper file - UINT8 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset = (row * origin->width) + startCol;
	  unsigned char *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned char uint8;
		if (fread (&uint8, sizeof (unsigned char), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = uint8;
	    }
      }
    return 1;
}

static int
read_ascii_int16 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, short *pixels)
{
/* reading from the Temporary helper file - INT16 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (short)) +
	      (startCol * sizeof (short));
	  short *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) < 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		short int16;
		if (fread (&int16, sizeof (short), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = int16;
	    }
      }
    return 1;
}

static int
read_ascii_uint16 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned int startRow,
		   unsigned int startCol, unsigned short *pixels)
{
/* reading from the Temporary helper file - UINT16 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (unsigned short)) +
	      (startCol * sizeof (unsigned short));
	  unsigned short *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned short uint16;
		if (fread (&uint16, sizeof (unsigned short), 1, origin->tmp) <=
		    0)
		    return 0;
		*p_out++ = uint16;
	    }
      }
    return 1;
}

static int
read_ascii_int32 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, int *pixels)
{
/* reading from the Temporary helper file - INT32 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (int)) + (startCol * sizeof (int));
	  int *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		int int32;
		if (fread (&int32, sizeof (int), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = int32;
	    }
      }
    return 1;
}

static int
read_ascii_uint32 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned int startRow,
		   unsigned int startCol, unsigned int *pixels)
{
/* reading from the Temporary helper file - UINT32 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (unsigned int)) +
	      (startCol * sizeof (unsigned int));
	  unsigned int *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned int uint32;
		if (fread (&uint32, sizeof (unsigned int), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = uint32;
	    }
      }
    return 1;
}

static int
read_ascii_float (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, float *pixels)
{
/* reading from the Temporary helper file - FLOAT */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (float)) +
	      (startCol * sizeof (float));
	  float *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		float flt;
		if (fread (&flt, sizeof (float), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = flt;
	    }
      }
    return 1;
}

static int
read_ascii_double (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned int startRow,
		   unsigned int startCol, double *pixels)
{
/* reading from the Temporary helper file - DOUBLE */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (double)) +
	      (startCol * sizeof (double));
	  double *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		double dbl;
		if (fread (&dbl, sizeof (double), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = dbl;
	    }
      }
    return 1;
}

static int
read_ascii_pixels (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned char sample_type,
		   unsigned int startRow, unsigned int startCol, void *pixels)
{
/* reading from the Temporary helper file */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  return read_ascii_int8 (origin, width, height, startRow, startCol,
				  (char *) pixels);
      case RL2_SAMPLE_UINT8:
	  return read_ascii_uint8 (origin, width, height, startRow, startCol,
				   (unsigned char *) pixels);
      case RL2_SAMPLE_INT16:
	  return read_ascii_int16 (origin, width, height, startRow, startCol,
				   (short *) pixels);
      case RL2_SAMPLE_UINT16:
	  return read_ascii_uint16 (origin, width, height, startRow, startCol,
				    (unsigned short *) pixels);
      case RL2_SAMPLE_INT32:
	  return read_ascii_int32 (origin, width, height, startRow, startCol,
				   (int *) pixels);
      case RL2_SAMPLE_UINT32:
	  return read_ascii_uint32 (origin, width, height, startRow, startCol,
				    (unsigned int *) pixels);
      case RL2_SAMPLE_FLOAT:
	  return read_ascii_float (origin, width, height, startRow, startCol,
				   (float *) pixels);
      case RL2_SAMPLE_DOUBLE:
	  return read_ascii_double (origin, width, height, startRow, startCol,
				    (double *) pixels);
      };
    return 0;
}

static int
read_from_ascii (rl2PrivAsciiOriginPtr origin, unsigned short width,
		 unsigned short height, unsigned char sample_type,
		 unsigned int startRow, unsigned int startCol,
		 unsigned char **pixels, int *pixels_sz)
{
/* creating a tile from the ASCII Grid origin */
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
    bufPixelsSz = width * height * pix_sz;
    bufPixels = malloc (bufPixelsSz);
    if (bufPixels == NULL)
	goto error;
    if ((startRow + height) > origin->height
	|| (startCol + width) > origin->width)
	rl2_prime_void_tile (bufPixels, width, height, sample_type, 1);

    if (!read_ascii_pixels
	(origin, width, height, sample_type, startRow, startCol, bufPixels))
	goto error;

    *pixels = bufPixels;
    *pixels_sz = bufPixelsSz;
    return RL2_OK;
  error:
    if (bufPixels != NULL)
	free (bufPixels);
    return RL2_ERROR;
}

RL2_DECLARE rl2RasterPtr
rl2_get_tile_from_ascii_origin (rl2CoveragePtr cvg, rl2AsciiOriginPtr ascii,
				unsigned int startRow, unsigned int startCol)
{
/* attempting to create a Coverage-tile from an ASCII Grid origin */
    unsigned int x;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    rl2RasterPtr raster = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    int unused_width = 0;
    int unused_height = 0;

    if (coverage == NULL || ascii == NULL)
	return NULL;
    if (rl2_eval_ascii_origin_compatibility (cvg, ascii) != RL2_TRUE)
	return NULL;
    if (origin->tmp == NULL)
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

/* attempting to create the tile */
    if (read_from_ascii
	(origin, coverage->tileWidth, coverage->tileHeight,
	 coverage->sampleType, startRow, startCol, &pixels,
	 &pixels_sz) != RL2_OK)
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
	  /* full Transparent mask */
	  memset (mask, 0, coverage->tileWidth * coverage->tileHeight);
	  for (row = 0; row < coverage->tileHeight; row++)
	    {
		unsigned char *p = mask + (row * coverage->tileWidth);
		if (row < shadow_y)
		  {
		      /* setting opaque pixels */
		      memset (p, 1, shadow_x);
		  }
	    }
      }
    raster =
	rl2_create_raster (coverage->tileWidth, coverage->tileHeight,
			   coverage->sampleType, RL2_PIXEL_DATAGRID, 1, pixels,
			   pixels_sz, NULL, mask, mask_size, NULL);
    if (raster == NULL)
	goto error;
    return raster;
  error:
    if (pixels != NULL)
	free (pixels);
    if (mask != NULL)
	free (mask);
    return NULL;
}
