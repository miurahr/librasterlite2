/*

 rasterlite2 -- main header file

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

The Original Code is the RasterLite2 library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2013
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

/**
 \file rasterlite2.h

 Main RasterLite2 header file
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _WIN32
#ifdef DLL_EXPORT
#define RL2_DECLARE __declspec(dllexport)
#else
#define RL2_DECLARE extern
#endif
#else
#define RL2_DECLARE __attribute__ ((visibility("default")))
#endif
#endif

#ifndef _RASTERLITE2_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RASTERLITE2_H
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef SPATIALITE_AMALGAMATION
#include <spatialite/sqlite3.h>
#else
#include <sqlite3.h>
#endif

#include <spatialite/gaiageo.h>

/** RasterLite2 flag: FALSE */
#define RL2_FALSE	0
/** RasterLite2 flag: TRUE */
#define RL2_TRUE	1

/** RasterLite2 ret-value: OK (success) */
#define RL2_OK		0
/** RasterLite2 ret-value: ERROR (failure) */
#define RL2_ERROR	-1

/** RasterLite2 constant: INVALID/UNDEFINED Pyramid Level */
#define RL_INVALID_PYRAMID_LEVEL	-1

/** RasterLite2 constant: Sample Type UNKNOWN */
#define RL2_SAMPLE_UNKNOWN	0xa0
/** RasterLite2 constant: Sample Type 1-bit */
#define RL2_SAMPLE_1_BIT	0xa1
/** RasterLite2 constant: Sample Type 2-bit */
#define RL2_SAMPLE_2_BIT	0xa2
/** RasterLite2 constant: Sample Type 4-bit */
#define RL2_SAMPLE_4_BIT	0xa3
/** RasterLite2 constant: Sample Type 8 bit Integer */
#define RL2_SAMPLE_INT8		0xa4
/** RasterLite2 constant: Sample Type 8 bit Unsigned Integer */
#define RL2_SAMPLE_UINT8	0xa5
/** RasterLite2 constant: Sample Type 16 bit Integer */
#define RL2_SAMPLE_INT16	0xa6
/** RasterLite2 constant: Sample Type 16 bit Unsigned Integer */
#define RL2_SAMPLE_UINT16	0xa7
/** RasterLite2 constant: Sample Type 32 bit Integer */
#define RL2_SAMPLE_INT32	0xa8
/** RasterLite2 constant: Sample Type 32 bit Unsigned Integer */
#define RL2_SAMPLE_UINT32	0xa9
/** RasterLite2 constant: Sample Type Floating Point Single Precision */
#define RL2_SAMPLE_FLOAT	0xaa
/** RasterLite2 constant: Sample Type Floating Point Double Precision */
#define RL2_SAMPLE_DOUBLE	0xab

/** RasterLite2 constant: Pixel Type UNKNOWN */
#define RL2_PIXEL_UNKNOWN	0x10
/** RasterLite2 constant: Pixel Type Monochrome - Bilevel */
#define RL2_PIXEL_MONOCHROME	0x11
/** RasterLite2 constant: Pixel Type Palette based */
#define RL2_PIXEL_PALETTE	0x12
/** RasterLite2 constant: Pixel Type Grayscale */
#define RL2_PIXEL_GRAYSCALE	0x13
/** RasterLite2 constant: Pixel Type Red-Green-Blue */
#define RL2_PIXEL_RGB		0x14
/** RasterLite2 constant: Pixel Type Multiband (arbitrary) */
#define RL2_PIXEL_MULTIBAND	0x15
/** RasterLite2 constant: Pixel Type Data-Grid */
#define RL2_PIXEL_DATAGRID	0x16

/** RasterLite2 constant: Opaque Pixel */
#define RL2_PIXEL_OPAQUE	0x80
/** RasterLite2 constant: Transparent Pixel */
#define RL2_PIXEL_TRANSPARENT	0x40

/** RasterLite2 constant: Compression UNKNOWN */
#define RL2_COMPRESSION_UNKNOWN	0x20
/** RasterLite2 constant: Compression None */
#define RL2_COMPRESSION_NONE	0x21
/** RasterLite2 constant: Compression Deflate (zip) */
#define RL2_COMPRESSION_DEFLATE	0x22
/** RasterLite2 constant: Compression LZMA */
#define RL2_COMPRESSION_LZMA	0x23
/** RasterLite2 constant: Compression GIF */
#define RL2_COMPRESSION_GIF	0x24
/** RasterLite2 constant: Compression PNG */
#define RL2_COMPRESSION_PNG	0x25
/** RasterLite2 constant: Compression JPEG */
#define RL2_COMPRESSION_JPEG	0x26
/** RasterLite2 constant: Compression WEBP (lossy mode) */
#define RL2_COMPRESSION_LOSSY_WEBP	0x27
/** RasterLite2 constant: Compression WEBP (lossless mode) */
#define RL2_COMPRESSION_LOSSLESS_WEBP	0x28
/** RasterLite2 constant: Compression CCITTFAX3 */
#define RL2_COMPRESSION_CCITTFAX3	0x29
/** RasterLite2 constant: Compression CCITTFAX4 */
#define RL2_COMPRESSION_CCITTFAX4	0x30
/** RasterLite2 constant: Compression LZW */
#define RL2_COMPRESSION_LZW	0x31

/** RasterLite2 constant: UNKNOWN number of Bands */
#define RL2_BANDS_UNKNOWN	0x00

/** RasterLite2 constant: Red Band */
#define RL2_RED_BAND		0
/** RasterLite2 constant: Green Band */
#define RL2_GREEN_BAND		1
/** RasterLite2 constant: Blue Band */
#define RL2_BLUE_BAND		2
/** RasterLite2 constant: Grayscale Band */
#define RL2_GRAYSCALE_BAND	0
/** RasterLite2 constant: Data-Grid Band */
#define RL2_DATAGRID_BAND	0
/** RasterLite2 constant: Monochrome Band */
#define RL2_MONOCHROME_BAND	0
/** RasterLite2 constant: Palette Band */
#define RL2_PALETTE_BAND	0

/** RasterLite2 constant: No Georeferencing infos */
#define RL2_GEOREFERENCING_NONE	-1234

/** RasterLite2 constant: Undefined Tile size */
#define RL2_TILESIZE_UNDEFINED	0

/** RasterLite2 constant: reproduction scale 1:1 */
#define RL2_SCALE_1	0x31
/** RasterLite2 constant: reproduction scale 1:2 */
#define RL2_SCALE_2	0x32
/** RasterLite2 constant: reproduction scale 1:4 */
#define RL2_SCALE_4	0x33
/** RasterLite2 constant: reproduction scale 1:8 */
#define RL2_SCALE_8	0x34

/**
 Typedef for RL2 Pixel object (opaque, hidden)

 \sa rl2PixelPtr
 */
    typedef struct rl2_pixel rl2Pixel;
/**
 Typedef for RL2 Pixel object pointer (opaque, hidden)

 \sa rl2Pixel
 */
    typedef rl2Pixel *rl2PixelPtr;

/**
 Typedef for RL2 Palette object (opaque, hidden)

 \sa rl2PalettePtr
 */
    typedef struct rl2_palette rl2Palette;
/**
 Typedef for RL2 Pixel object pointer (opaque, hidden)

 \sa rl2Palette
 */
    typedef rl2Palette *rl2PalettePtr;

/**
 Typedef for RL2 Raster object (opaque, hidden)

 \sa rl2RasterPtr
 */
    typedef struct rl2_raster rl2Raster;
/**
 Typedef for RL2 Raster object pointer (opaque, hidden)

 \sa rl2Raster
 */
    typedef rl2Raster *rl2RasterPtr;

/**
 Typedef for RL2 Section object (opaque, hidden)

 \sa rl2SectionPtr
 */
    typedef struct rl2_section rl2Section;
/**
 Typedef for RL2 Section object pointer (opaque, hidden)

 \sa rl2Section
 */
    typedef rl2Section *rl2SectionPtr;

/**
 Typedef for RL2 Coverage object (opaque, hidden)

 \sa rl2CoveragePtr
 */
    typedef struct rl2_coverage rl2Coverage;
/**
 Typedef for RL2 Coverage object pointer (opaque, hidden)

 \sa rl2Coverage
 */
    typedef rl2Coverage *rl2CoveragePtr;

/**
 Typedef for RL2 TIFF Origin object (opaque, hidden)

 \sa rl2TiffOriginPtr
 */
    typedef struct rl2_tiff_origin rl2TiffOrigin;
/**
 Typedef for RL2 TIFF Origin object pointer (opaque, hidden)

 \sa rl2TiffOrigin
 */
    typedef rl2TiffOrigin *rl2TiffOriginPtr;

/**
 Typedef for RL2 TIFF Destination object (opaque, hidden)

 \sa rl2TiffDestinationPtr
 */
    typedef struct rl2_tiff_destination rl2TiffDestination;
/**
 Typedef for RL2 TIFF Destination object pointer (opaque, hidden)

 \sa rl2TiffDestination
 */
    typedef rl2TiffDestination *rl2TiffDestinationPtr;

/**
 Releases (frees) dynamic memory allocated by RasterLite2

 \param p pointer to the dynamic memory block to be released.
 */
    RL2_DECLARE void rl2_free (void *ptr);

/**
 Allocates and initializes a new Pixel object

 \param sample_type one of RL2_SAMPLE_1_BIT, RL2_SAMPLE_2_BIT, RL2_SAMPLE_4_BIT,
        RL2_SAMPLE_INT8, RL2_SAMPLE_UINT8, RL2_SAMPLE_INT16, RL2_SAMPLE_UINT16,
		RL2_SAMPLE_INT32, RL2_SAMPLE_UINT32, RL2_SAMPLE_FLOAT or RL2_SAMPLE_DOUBLE.
 \param pixel_type one of RL2_PIXEL_MONOCHROME, RL2_PIXEL_PALETTE, RL2_PIXEL_GRAYSCALE,
		RL2_PIXEL_RGB, RL2_PIXEL_MULTIBAND, RL2_PIXEL_DATAGRID.
 \param num_samples number of samples per pixel (aka Bands)
 
 \return the pointer to newly created Coverage Object: NULL on failure.
 
 \sa rl2_destroy_pixel, rl2_compare_pixels, rl2_get_pixel_type, 
		rl2_get_pixel_sample_1bit,
		rl2_set_pixel_sample_1bit, rl2_get_pixel_sample_2bit,
		rl2_set_pixel_sample_2bit, rl2_get_pixel_sample_4bit,
		rl2_set_pixel_sample_4bit, rl2_get_pixel_sample_int8,
		rl2_set_pixel_sample_uint8, rl2_get_pixel_sample_int16,
		rl2_set_pixel_sample_uint16, rl2_get_pixel_sample_int32,
		rl2_set_pixel_sample_uint32, rl2_get_pixel_sample_float,
		rl2_set_pixel_sample_float, rl2_get_pixel_sample_double,
		rl2_set_pixel_sample_double, rl2_is_pixel_transparent,
		rl2_is_pixel_opaque, rl2_set_pixel_transparent,
		rl2_set_pixel_opaque
 
 \note you are responsible to destroy (before or after) any allocated 
 Pixel object.
 */
    RL2_DECLARE rl2PixelPtr
	rl2_create_pixel (unsigned char sample_type, unsigned char pixel_type,
			  unsigned char num_samples);

/**
 Destroys a Pixel Object

 \param pxl pointer to object to be destroyed

 \sa rl2_create_pixel
 */
    RL2_DECLARE void rl2_destroy_pixel (rl2PixelPtr pxl);

/**
 Testing if two different Pixels are exactly the same

 \param pxl1 pointer to the first Pixel Object.
 \param pxl2 pointer to the second Pixel Object.
 
 \return RL2_TRUE or RL2_FALSE; RL2_ERROR if any error is encountered.

 \sa rl2_create_pixel
 */
    RL2_DECLARE int rl2_compare_pixels (rl2PixelPtr pxl1, rl2PixelPtr pxl2);

/**
 Retrieving the Sample Type from a Pixel Object

 \param pxl pointer to the Pixel Object.
 \param sample_type on completion the variable referenced by this
 pointer will contain the Sampe Type.
 \param pixel_type on completion the variable referenced by this
 pointer will contain the Pixel Type.
 \param num_bands on completion the variable referenced by this
 pointer will contain the Number of Bands.
 
 \return  RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_pixel
 */
    RL2_DECLARE int rl2_get_pixel_type (rl2PixelPtr pxl,
					unsigned char *sample_type,
					unsigned char *pixel_type,
					unsigned char *num_bands);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [1 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_1bit
 */
    RL2_DECLARE int rl2_get_pixel_sample_1bit (rl2PixelPtr pxl,
					       unsigned char *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [1 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_1bit
 */
    RL2_DECLARE int rl2_set_pixel_sample_1bit (rl2PixelPtr pxl,
					       unsigned char sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [2 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_2bit
 */
    RL2_DECLARE int rl2_get_pixel_sample_2bit (rl2PixelPtr pxl,
					       unsigned char *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [2 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_2bit
 */
    RL2_DECLARE int rl2_set_pixel_sample_2bit (rl2PixelPtr pxl,
					       unsigned char sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [4 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_4bit
 */
    RL2_DECLARE int rl2_get_pixel_sample_4bit (rl2PixelPtr pxl,
					       unsigned char *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [4 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_4bit
 */
    RL2_DECLARE int rl2_set_pixel_sample_4bit (rl2PixelPtr pxl,
					       unsigned char sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [integer, 8 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_int8
 */
    RL2_DECLARE int rl2_get_pixel_sample_int8 (rl2PixelPtr pxl, char *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [integer, 8 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_int8
 */
    RL2_DECLARE int rl2_set_pixel_sample_int8 (rl2PixelPtr pxl, char sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [unsigned integer, 8 bit sample]

 \param pxl pointer to the Pixel Object.
 \param band the Sample/Band index (the first sample corresponds to index ZERO).
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_uint8
 */
    RL2_DECLARE int rl2_get_pixel_sample_uint8 (rl2PixelPtr pxl, int band,
						unsigned char *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [unsigned integer, 8 bit sample]

 \param pxl pointer to the Pixel Object.
 \param band the Sample/Band index (the first sample corresponds to index ZERO).
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_uint8
 */
    RL2_DECLARE int rl2_set_pixel_sample_uint8 (rl2PixelPtr pxl, int band,
						unsigned char sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [integer, 16 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_int16
 */
    RL2_DECLARE int rl2_get_pixel_sample_int16 (rl2PixelPtr pxl, short *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [integer, 16 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_int16
 */
    RL2_DECLARE int rl2_set_pixel_sample_int16 (rl2PixelPtr pxl, short sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [unsigned integer, 16 bit sample]

 \param pxl pointer to the Pixel Object.
 \param band the Sample/Band index (the first sample corresponds to index ZERO).
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_uint16
 */
    RL2_DECLARE int rl2_get_pixel_sample_uint16 (rl2PixelPtr pxl, int band,
						 unsigned short *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [unsigned integer, 16 bit sample]

 \param pxl pointer to the Pixel Object.
 \param band the Sample/Band index (the first sample corresponds to index ZERO).
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_uint16
 */
    RL2_DECLARE int rl2_set_pixel_sample_uint16 (rl2PixelPtr pxl, int band,
						 unsigned short sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [integer, 32 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_int32
 */
    RL2_DECLARE int rl2_get_pixel_sample_int32 (rl2PixelPtr pxl, int *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [integer, 32 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_int32
 */
    RL2_DECLARE int rl2_set_pixel_sample_int32 (rl2PixelPtr pxl, int sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [unsigned integer, 32 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_uint32
 */
    RL2_DECLARE int rl2_get_pixel_sample_uint32 (rl2PixelPtr pxl,
						 unsigned int *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [unsigned integer, 32 bit sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_uint32
 */
    RL2_DECLARE int rl2_set_pixel_sample_uint32 (rl2PixelPtr pxl,
						 unsigned int sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [floating point, single precision sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_float
 */
    RL2_DECLARE int rl2_get_pixel_sample_float (rl2PixelPtr pxl, float *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [floating point, single precision sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_float
 */
    RL2_DECLARE int rl2_set_pixel_sample_float (rl2PixelPtr pxl, float sample);

/**
 Retrieving the Pixel/Sample value from a Pixel Object [floating point, double precision sample]

 \param pxl pointer to the Pixel Object.
 \param sample on completion the variable referenced by this
 pointer will contain the Pixel/Sampe Value.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including querying a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_set_pixel_sample_double
 */
    RL2_DECLARE int rl2_get_pixel_sample_double (rl2PixelPtr pxl,
						 double *sample);

/**
 Assigning the Pixel/Sample value to a Pixel Object [floating point, double precision sample]

 \param pxl pointer to the Pixel Object.
 \param sample the Pixel/Sampe Value to be set.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing a Pixel of mismatching PixelType).

 \sa rl2_create_pixel, rl2_get_pixel_type, rl2_get_pixel_sample_double
 */
    RL2_DECLARE int rl2_set_pixel_sample_double (rl2PixelPtr pxl,
						 double sample);

/**
 Testing if a Pixel Object is Transparent

 \param pxl pointer to the Pixel Object.
 \param is_transparent on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_pixel, rl2_is_pixel_opaque, rl2_set_pixel_transparent, 
 rl2_set_pixel_opaque
 */
    RL2_DECLARE int rl2_is_pixel_transparent (rl2PixelPtr pxl,
					      int *is_transparent);

/**
 Testing if a Pixel Object is Opaque

 \param pxl pointer to the Pixel Object.
 \param is_opaque on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_pixel, rl2_is_pixel_transparent, rl2_set_pixel_transparent, 
 rl2_set_pixel_opaque
 */
    RL2_DECLARE int rl2_is_pixel_opaque (rl2PixelPtr pxl, int *is_opaque);

/**
 Forcing a Pixel Object to be Transparent

 \param pxl pointer to the Pixel Object.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_pixel, rl2_is_pixel_opaque, rl2_is_pixel_transparent, 
 rl2_set_pixel_opaque
 */
    RL2_DECLARE int rl2_set_pixel_transparent (rl2PixelPtr pxl);

/**
 Forcing a Pixel Object to be Opaque

 \param pxl pointer to the Pixel Object.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_pixel, rl2_is_pixel_opaque, rl2_is_pixel_transparent, 
 rl2_set_pixel_transparent
 */
    RL2_DECLARE int rl2_set_pixel_opaque (rl2PixelPtr pxl);

/**
 Allocates and initializes a new Palette object

 \param num_entries total number of color entries for this Palette:
 any valid palette should contain at least one entry and no more than 256 entries.
 
 \return the pointer to newly created Palette Object: NULL on failure.
 
 \sa rl2_destroy_palette, rl2_set_palette_color, rl2_get_palette_index,
	rl2_set_palette_hexrgb, rl2_get_palette_entries, rl2_get_palette_colors,
	rl2_get_palette_type
 
 \note you are responsible to destroy (before or after) any allocated 
 Palette object.
 */
    RL2_DECLARE rl2PalettePtr rl2_create_palette (int num_entries);

/**
 Destroys a Palette Object

 \param plt pointer to object to be destroyed

 \sa rl2_create_palette
 */
    RL2_DECLARE void rl2_destroy_palette (rl2PalettePtr plt);

/**
 Assigns a Palette Color

 \param plt pointer to the Palette Object
 \param index references a Palette Entry [first Entry has index ZERO]
 \param r Red component
 \param g Green component
 \param b Blue component
 \param alpha Alpha component
 
 \return RL2_OK on success: RL2_ERROR on failure (invald Palette or invalid Index).

 \sa rl2_create_palette, rl2_set_palette_hexrgb, rl2_get_palette_index, 
	rl2_get_palette_colors, rl2_get_palette_entries
 */
    RL2_DECLARE int
	rl2_set_palette_color (rl2PalettePtr plt, int index,
			       unsigned char r, unsigned char g,
			       unsigned char b, unsigned char alpha);

/**
 Assigns a Palette Color (Hex RGB)

 \param plt pointer to the Palette Object
 \param index references a Palette Entry [first Entry has index ZERO]
 \param rgb an Hex RGB Color: "#ff0000" corresponds to Red, "#0000ff"
 corresponds to Blue and so on.
 
 \return RL2_OK on success: RL2_ERROR on failure (invald Palette or invalid Index).

 \sa rl2_create_palette, rl2_set_palette_color, rl2_get_palette_index, 
	rl2_get_palette_colors, rl2_get_palette_entries
 */
    RL2_DECLARE int
	rl2_set_palette_hexrgb (rl2PalettePtr plt, int index, const char *rgb);

/**
 Return the Palette Index corresponding to a given Color

 \param plt pointer to the Palette Object
 \param index on completion the variable referenced by this
 pointer will contain the Index corresponding to the matching Entry.
 \param r Red component
 \param g Green component
 \param b Blue component
 \param alpha Alpha component
 
 \return RL2_OK on success: RL2_ERROR on failure (invald Palette or no matching Entry).

 \sa rl2_create_palette
 */
    RL2_DECLARE int
	rl2_get_palette_index (rl2PalettePtr plt, unsigned char *index,
			       unsigned char r, unsigned char g,
			       unsigned char b, unsigned char alpha);

/**
 Retrieving the total number of Palette Entries

 \param plt pointer to the Palette Object.
 \param num_entries on completion the variable referenced by this
 pointer will contain the total count of Colors.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_palette, rl2_get_palette_colors, rl2_set_palette_color,
	rl2_set_palette_hexrgb
 */
    RL2_DECLARE int rl2_get_palette_entries (rl2PalettePtr plt,
					     unsigned short *num_entries);

/**
 Exports all Palette Colors as separate arrays for each component

 \param plt pointer to the Palette Object
 \param num_entries on completion the variable referenced by this
 pointer will contain the total count of Colors (i.e. the size of each
 one of the following arrays).
 \param r on completion will point to an array of Red components
 \param g on completion will point to an array of Green components
 \param b on completion will point to an array of Blue components
 \param alpha on completion will point to an array of Alpha components
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_palette
 
 \note you are responsible to destroy (before or after) any array
 created by rl2_get_palette_colors() by invoking rl2_free().
 */
    RL2_DECLARE int
	rl2_get_palette_colors (rl2PalettePtr plt, unsigned short *num_entries,
				unsigned char **r, unsigned char **g,
				unsigned char **b, unsigned char **alpha);

/**
 Return the best fit Sample Type and Pixel Type for a given Palette Object

 \param plt pointer to the Palette Object
 \param sample_type on completion the variable referenced by this
 pointer will contain the Sample Type corresponding to the Palette Object
 (one of RL2_SAMPLE_1_BIT, RL2_SAMPLE_2_BIT, RL2_SAMPLE_4_BIT or RL2_SAMPLE_UINT8)
 \param pixel_type on completion the variable referenced by this
 pointer will contain the Pixel Type corresponding to the Palette Object
 (one of RL2_PIXEL_PALETTE, RL2_PIXEL_MONOCHROME or RL2_PIXEL_GRAYSCALE)
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_palette
 */
    RL2_DECLARE int
	rl2_get_palette_type (rl2PalettePtr plt, unsigned char *sample_type,
			      unsigned char *pixel_type);

/**
 Allocates and initializes a new Coverage object

 \param name a text string intended to be the symbolic Coverage name.
 \param sample_type one of RL2_SAMPLE_1_BIT, RL2_SAMPLE_2_BIT, RL2_SAMPLE_4_BIT,
        RL2_SAMPLE_INT8, RL2_SAMPLE_UINT8, RL2_SAMPLE_INT16, RL2_SAMPLE_UINT16,
		RL2_SAMPLE_INT32, RL2_SAMPLE_UINT32, RL2_SAMPLE_FLOAT or RL2_SAMPLE_DOUBLE.
 \param pixel_type one of RL2_PIXEL_MONOCHROME, RL2_PIXEL_PALETTE, RL2_PIXEL_GRAYSCALE,
		RL2_PIXEL_RGB, RL2_PIXEL_MULTIBAND, RL2_PIXEL_DATAGRID.
 \param num_samples number of samples per pixel (aka Bands)
 \param compression one of RL2_COMPRESSION_NONE, RL2_COMPRESSION_DEFLATE,
		RL2_COMPRESSION_LZMA, RL2_COMPRESSION_GIF, RL2_COMPRESSION_PNG,
		RL2_COMPRESSION_JPEG, RL2_COMPRESSION_LOSSY_WEBP or
 		RL2_COMPRESSION_LOSSLESS_WEBP
 \param quality compression quality factor (0-100); only meaningfull for
		JPEG or WEBP lossy compressions, ignored in any other case.
 \param tile_width the individual tile width in pixels.
 \param tile_height the individual tile height in pixels.
 \param no_data pointer to a Pixel Object indented as NO-DATA value;
		could be eventually NULL if not required.
 
 \return the pointer to newly created Coverage Object: NULL on failure.
 
 \sa rl2_destroy_coverage, rl2_coverage_georeference, rl2_get_coverage_name,
		rl2_get_coverage_type, rl2_get_coverage_compression, 
		rl2_is_coverage_uncompressed, rl2_is_coverage_compression_lossless, 
		rl2_is_coverage_compression_lossy, rl2_get_coverage_tile_size,
		rl2_get_coverage_no_data, rl2_create_coverage_pixel, 
		rl2_get_coverage_srid
 
 \note you are responsible to destroy (before or after) any allocated 
 Coverage object.
 */
    RL2_DECLARE rl2CoveragePtr
	rl2_create_coverage (const char *name, unsigned char sample_type,
			     unsigned char pixel_type,
			     unsigned char num_samples,
			     unsigned char compression, int quality,
			     unsigned short tile_width,
			     unsigned short tile_height, rl2PixelPtr no_data);

/**
 Destroys a Coverage Object

 \param cvg pointer to object to be destroyed

 \sa rl2_create_coverage
 */
    RL2_DECLARE void rl2_destroy_coverage (rl2CoveragePtr cvg);

/**
 Assigns GeoReferencing parameters to a Coverage Object

 \param cvg pointer to the Coverage Object.
 \param srid a valid SRID identifier
 \param horz_res horizontal pixel resolution: the size (measured
		in map units) corresponding to a single Pixel at full
		resolution.
 \param vert_res vertical pixel resolution.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int
	rl2_coverage_georeference (rl2CoveragePtr cvg, int srid,
				   double horz_res, double vert_res);

/**
 Retrieving the Name from a Coverage Object

 \param cvg pointer to the Coverage Object.
 
 \return pointer to the Name text string; NULL if any error is encountered.

 \sa rl2_create_coverage
 */
    RL2_DECLARE const char *rl2_get_coverage_name (rl2CoveragePtr cvg);

/**
 Retrieving the Sample Type from a Coverage Object

 \param cvg pointer to the Coverage Object.
 \param sample_type on completion the variable referenced by this
 pointer will contain the Sampe Type.
 \param pixel_type on completion the variable referenced by this
 pointer will contain the Pixel Type.
 \param num_bands on completion the variable referenced by this
 pointer will contain the Number of Bands.
 
 \return  RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int rl2_get_coverage_type (rl2CoveragePtr cvg,
					   unsigned char *sample_type,
					   unsigned char *pixel_type,
					   unsigned char *num_bands);

/**
 Retrieving the Compression Type from a Coverage Object

 \param cvg pointer to the Coverage Object.
 \param num_bands on completion the variable referenced by this
 pointer will contain the Compression Type.
 \param num_bands on completion the variable referenced by this
 pointer will contain the Compression Quality.
 
 \return  RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int rl2_get_coverage_compression (rl2CoveragePtr cvg,
						  unsigned char *compression,
						  int *quality);

/**
 Testing if a Coverage Object is uncompressed or compressed

 \param cvg pointer to the Coverage Object.
 \param is_uncompressed on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int rl2_is_coverage_uncompressed (rl2CoveragePtr cvg,
						  int *is_uncompressed);

/**
 Testing if a Coverage Object adopts a lossless compression

 \param cvg pointer to the Coverage Object.
 \param is_lossless on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int rl2_is_coverage_compression_lossless (rl2CoveragePtr cvg,
							  int *is_lossless);

/**
 Testing if a Coverage Object adopts a lossy compression

 \param cvg pointer to the Coverage Object.
 \param is_lossy on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int rl2_is_coverage_compression_lossy (rl2CoveragePtr cvg,
						       int *is_lossy);

/**
 Retrieving the Tile Size from a Coverage Object

 \param cvg pointer to the Coverage Object.
 \param tile_width on completion the variable referenced by this
 pointer will contain the coverage's Tile Width.
 \param tile_height on completion the variable referenced by this
 pointer will contain the coverage's Tile Height.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int rl2_get_coverage_tile_size (rl2CoveragePtr cvg,
						unsigned short *tile_width,
						unsigned short *tile_height);

/**
 Retrieving the NO-DATA Pixel value from a Coverage Object

 \param cvg pointer to the Coverage Object.
 
 \return pointer to the Pixel Object representing NO-DATA value; 
  NULL if any error is encountered or if the Coverage has no NO-DATA value.

 \sa rl2_create_coverage
 */
    RL2_DECLARE rl2PixelPtr rl2_get_coverage_no_data (rl2CoveragePtr cvg);

/**
 Creates a new Pixel Object suitable for a given Coverage Object

 \param cvg pointer to the Coverage Object.
 
 \return pointer to the newly created Pixel Object; NULL if any error is encountered.

 \sa rl2_create_coverage, rl2_create_pixel, rl2_destroy_pixel
 
 \note you are responsible to destroy (before or after) the allocated 
 Pixel object.
 */
    RL2_DECLARE rl2PixelPtr rl2_create_coverage_pixel (rl2CoveragePtr rst);

/**
 Retrieving the SRID from a Coverage Object

 \param cvg pointer to the Coverage Object.
 \param srid on completion the variable referenced by this
 pointer will contain the coverage's SRID.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_coverage
 */
    RL2_DECLARE int rl2_get_coverage_srid (rl2CoveragePtr cvg, int *srid);

/**
 Allocates and initializes a new Section object

 \param name a text string intended to be the symbolic Section name.
 \param compression one of RL2_COMPRESSION_NONE, RL2_COMPRESSION_DEFLATE,
		RL2_COMPRESSION_LZMA, RL2_COMPRESSION_GIF, RL2_COMPRESSION_PNG,
		RL2_COMPRESSION_JPEG, RL2_COMPRESSION_LOSSY_WEBP or
 		RL2_COMPRESSION_LOSSLESS_WEBP
 \param tile_width the individual tile width in pixels.
 \param tile_height the individual tile height in pixels.
 \param rst pointer to a Raster Object.
 
 \return the pointer to newly created Section Object: NULL on failure.
 
 \sa rl2_destroy_section, rl2_get_section_name, rl2_get_section_compression, 
		rl2_is_section_uncompressed, rl2_is_section_compression_lossless, 
		rl2_is_section_compression_lossy, rl2_get_section_tile_width, 
		rl2_get_section_tile_height, rl2_get_section_raster,
		rl2_section_from_gif, rl2_section_from_png, rl2_section_from_jpeg,
		rl2_section_from_webp, rl2_section_to_gif, rl2_section_to_png,
		rl2_section_to_jpeg, rl2_section_to_lossy_webp,
		rl2_section_to_lossless_webp
 
 \note you are responsible to destroy (before or after) any allocated 
 Section object.
 */
    RL2_DECLARE rl2SectionPtr
	rl2_create_section (const char *name, unsigned char compression,
			    unsigned short tile_width,
			    unsigned short tile_height, rl2RasterPtr rst);

/**
 Allocates and initializes a new Section object from an external GIF image

 \param path pathname leading to the external GIF image.
 
 \return the pointer to newly created Section Object: NULL on failure.
 
 \sa rl2_destroy_section, rl2_create_section, rl2_section_to_gif
 
 \note you are responsible to destroy (before or after) any allocated 
 Section object.
 */
    RL2_DECLARE rl2SectionPtr rl2_section_from_gif (const char *path);

/**
 Allocates and initializes a new Section object from an external PMG image

 \param path pathname leading to the external PNG image.
 
 \return the pointer to newly created Section Object: NULL on failure.
 
 \sa rl2_destroy_section, rl2_create_section, rl2_section_to_png
 
 \note you are responsible to destroy (before or after) any allocated 
 Section object.
 */
    RL2_DECLARE rl2SectionPtr rl2_section_from_png (const char *path);

/**
 Allocates and initializes a new Section object from an external JPEG image

 \param path pathname leading to the external JPEG image.
 
 \return the pointer to newly created Section Object: NULL on failure.
 
 \sa rl2_destroy_section, rl2_create_section, rl2_section_to_jpeg
 
 \note you are responsible to destroy (before or after) any allocated 
 Section object.
 */
    RL2_DECLARE rl2SectionPtr rl2_section_from_jpeg (const char *path);

/**
 Allocates and initializes a new Section object from an external WEBP image

 \param path pathname leading to the external WEBP image.
 
 \return the pointer to newly created Section Object: NULL on failure.
 
 \sa rl2_destroy_section, rl2_create_section, rl2_section_to_lossy_webp,
	rl2_section_to_lossless_webp
 
 \note you are responsible to destroy (before or after) any allocated 
 Section object.
 */
    RL2_DECLARE rl2SectionPtr rl2_section_from_webp (const char *path);

/**
 Exports a Section object as an external GIF image

 \param scn pointer to the Section Object.
 \param path pathname leading to the external GIF image.
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_section, rl2_section_from_gif
 */
    RL2_DECLARE int rl2_section_to_gif (rl2SectionPtr scn, const char *path);

/**
 Exports a Section object as an external PNG image

 \param scn pointer to the Section Object.
 \param path pathname leading to the external PNG image.
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_section, rl2_section_from_png
 */
    RL2_DECLARE int rl2_section_to_png (rl2SectionPtr scn, const char *path);

/**
 Exports a Section object as an external JPEG image

 \param scn pointer to the Section Object.
 \param path pathname leading to the external JPEG image.
 \param quality compression quality factor (0-100).
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_section, rl2_section_from_jpeg
 */
    RL2_DECLARE int
	rl2_section_to_jpeg (rl2SectionPtr scn, const char *path, int quality);

/**
 Exports a Section object as an external WEBP (lossy) image

 \param scn pointer to the Section Object.
 \param path pathname leading to the external JPEG image.
 \param quality compression quality factor (0-100).
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_section, rl2_section_from_jpeg
 */
    RL2_DECLARE int rl2_section_to_lossy_webp (rl2SectionPtr scn,
					       const char *path, int quality);

/**
 Exports a Section object as an external WEBP (lossless) image

 \param scn pointer to the Section Object.
 \param path pathname leading to the external WEBP image.
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_section, rl2_section_from_webp
 */
    RL2_DECLARE int rl2_section_to_lossless_webp (rl2SectionPtr scn,
						  const char *path);

/**
 Destroys a Section Object

 \param scn pointer to object to be destroyed

 \sa rl2_create_section
 */
    RL2_DECLARE void rl2_destroy_section (rl2SectionPtr scn);

/**
 Retrieving the Name from a Section Object

 \param scn pointer to the Section Object.
 
 \return pointer to the Name text string; NULL if any error is encountered.

 \sa rl2_create_section
 */
    RL2_DECLARE const char *rl2_get_section_name (rl2SectionPtr scn);

/**
 Retrieving the Compression Type from a Section Object

 \param scn pointer to the Section Object.
 \param compression on completion the variable referenced by this
 pointer will contain the section's Compression Type.
 
 \return  RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_section
 */
    RL2_DECLARE int rl2_get_section_compression (rl2SectionPtr scn,
						 unsigned char *compression);

/**
 Testing if a Section Object is uncompressed or compressed

 \param scn pointer to the Section Object.
 \param is_uncompressed on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_section
 */
    RL2_DECLARE int rl2_is_section_uncompressed (rl2SectionPtr scn,
						 int *is_uncompressed);

/**
 Testing if a Section Object adopts a lossless compression

 \param scn pointer to the Section Object.
 \param is_lossless on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_section
 */
    RL2_DECLARE int rl2_is_section_compression_lossless (rl2SectionPtr scn,
							 int *is_lossless);

/**
 Testing if a Section Object adopts a lossy compression

 \param scn pointer to the Section Object.
 \param is_lossy on completion the variable referenced by this
 pointer will contain RL2_TRUE or RL2_FALSE.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_section
 */
    RL2_DECLARE int rl2_is_section_compression_lossy (rl2SectionPtr scn,
						      int *is_lossy);

/**
 Retrieving the Tile Size from a Section Object

 \param scn pointer to the Section Object.
 \param tile_width on completion the variable referenced by this
 pointer will contain the section's Tile Width.
 \param tile_height on completion the variable referenced by this
 pointer will contain the section's Tile Height.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_section
 */
    RL2_DECLARE int rl2_get_section_tile_size (rl2SectionPtr scn,
					       unsigned short *tile_width,
					       unsigned short *tile_height);

/**
 Return a reference to the Raster Object encapsulated within a Section Object

 \param cvg pointer to the Section Object.
 
 \return pointer to the Raster Object; NULL if any error is encountered.

 \sa rl2_create_section
 
 \note full ownership of the referenced Raster Object will still belong to the Section Object.
 */
    RL2_DECLARE rl2RasterPtr rl2_get_section_raster (rl2SectionPtr scn);

/**
 Allocates and initializes a new Raster object

 \param width Raster Width (in pixels).
 \param height Raster Height (in pixels).
 \param sample_type one of RL2_SAMPLE_1_BIT, RL2_SAMPLE_2_BIT, RL2_SAMPLE_4_BIT,
        RL2_SAMPLE_INT8, RL2_SAMPLE_UINT8, RL2_SAMPLE_INT16, RL2_SAMPLE_UINT16,
		RL2_SAMPLE_INT32, RL2_SAMPLE_UINT32, RL2_SAMPLE_FLOAT or RL2_SAMPLE_DOUBLE.
 \param pixel_type one of RL2_PIXEL_MONOCHROME, RL2_PIXEL_PALETTE, RL2_PIXEL_GRAYSCALE,
		RL2_PIXEL_RGB, RL2_PIXEL_MULTIBAND, RL2_PIXEL_DATAGRID.
 \param num_samples number of samples per pixel (aka Bands)
 \param bufpix pointer to the Buffer containing all Pixels
 \param bufpix_size size (in bytes) of the above Buffer
 \param palette pointer to a Palette object (NULL is the Pixel Type doesn't require a Palette)
 \param mask pointer to an optional Transparency Mask (may be NULL if no Mask is required)
 \param mask_size size (in bytes) of the above Mask (ZERO if no Mask is required)
 \param no_data pointer to a Pixel Object indented as NO-DATA value;
		could be eventually NULL if not required.
 
 \return the pointer to newly created Raster Object: NULL on failure.
 
 \sa rl2_destroy_raster, rl2_get_raster_width, rl2_get_raster_height,
		rl2_get_raster_raster_type, rl2_get_raster_pixel_type,
		rl2_get_raster_bands, rl2_get_raster_no_data, rl2_get_raster_srid,
		rl2_get_raster_horizontal_resolution, rl2_get_raster_vertical_resolution,
		rl2_get_raster_minX, rl2_get_raster_minY, rl2_get_raster_maxX,
		rl2_get_raster_maxY, rl2_create_raster_pixel,
		rl2_raster_georeference_center, rl2_raster_georeference_upper_left,
		rl2_raster_georeference_lower_left, rl2_raster_georeference_upper_right,
		rl2_raster_georeference_lower_right, rl2_raster_georeference_frame,
		rl2_raster_data_to_1bit, rl2_raster_data_to_2bit, rl2_raster_data_to_4bit,
		rl2_raster_data_to_int8, rl2_raster_data_to_uint8, rl2_raster_data_to_int16, 
		rl2_raster_data_to_uint16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
		rl2_raster_data_to_float, rl2_raster_data_to_double, rl2_raster_band_to_uint8, 
		rl2_raster_band_to_uint16, rl2_raster_bands_to_RGB, rl2_raster_to_gif,
		rl2_raster_to_png, rl2_raster_to_jpeg, rl2_raster_to_lossless_webp,
		rl2_raster_to_lossy_webp, rl2_raster_from_gif, rl2_raster_from_png, 
		rl2_raster_from_jpeg, rl2_raster_from_webp 
 
 \note you are responsible to destroy (before or after) any allocated 
 Raster object.
 */
    RL2_DECLARE rl2RasterPtr
	rl2_create_raster (unsigned short width, unsigned short height,
			   unsigned char sample_type, unsigned char pixel_type,
			   unsigned char num_samples, unsigned char *bufpix,
			   int bufpix_size, rl2PalettePtr palette,
			   unsigned char *mask, int mask_size,
			   rl2PixelPtr no_data);

/**
 Destroys a Raster Object

 \param rst pointer to object to be destroyed

 \sa rl2_create_raster
 */
    RL2_DECLARE void rl2_destroy_raster (rl2RasterPtr rst);

/**
 Retrieving the Width dimension from a Raster Object

 \param rst pointer to the Raster Object.
 \param width on completion the variable referenced by this
 pointer will contain the raster's Width.
 \param height on completion the variable referenced by this
 pointer will contain the raster's THeight.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster
 */
    RL2_DECLARE int rl2_get_raster_size (rl2RasterPtr rst,
					 unsigned short *width,
					 unsigned short *height);

/**
 Retrieving the Sample Type from a Raster Object

 \param cvg pointer to the Raster Object.
 \param sample_type on completion the variable referenced by this
 pointer will contain the Sampe Type.
 \param pixel_type on completion the variable referenced by this
 pointer will contain the Pixel Type.
 \param num_bands on completion the variable referenced by this
 pointer will contain the Number of Bands.
 
 \return  RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster
 */
    RL2_DECLARE int rl2_get_raster_type (rl2RasterPtr rst,
					 unsigned char *sample_type,
					 unsigned char *pixel_type,
					 unsigned char *num_bands);

/**
 Retrieving the NO-DATA Pixel value from a Raster Object

 \param rst pointer to the Raster Object.
 
 \return pointer to the Pixel Object representing NO-DATA value; 
  NULL if any error is encountered or if the Raster has no NO-DATA value.

 \sa rl2_create_raster
 */
    RL2_DECLARE rl2PixelPtr rl2_get_raster_no_data (rl2RasterPtr rst);

/**
 Retrieving the SRID from a Raster Object

 \param rst pointer to the Raster Object.
 \param srid on completion the variable referenced by this
 pointer will contain the raster's SRID.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster
 */
    RL2_DECLARE int rl2_get_raster_srid (rl2RasterPtr rst, int *srid);

/**
 Retrieving the Pixel resolution from a Raster Object

 \param rst pointer to the Raster Object.
 \param hResolution on completion the variable referenced by this
 pointer will contain the raster's Horizontal Resolution.
 \param vResolution on completion the variable referenced by this
 pointer will contain the raster's Vertical Resolution.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster
 */
    RL2_DECLARE int rl2_get_raster_resolution (rl2RasterPtr rst,
					       double *hResolution,
					       double *vResolution);

/**
 Retrieving the full extent from a Raster Object

 \param rst pointer to the Raster Object.
 \param minX on completion the variable referenced by this
 pointer will contain the raster's minimum X coordinate.
 \param minY on completion the variable referenced by this
 pointer will contain the raster's minimum Y coordinate.
 \param maxX on completion the variable referenced by this
 pointer will contain the raster's maximum X coordinate.
 \param maxY on completion the variable referenced by this
 pointer will contain the raster's maximum Y coordinate.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster
 */
    RL2_DECLARE int rl2_get_raster_extent (rl2RasterPtr rst,
					   double *minX, double *minY,
					   double *maxX, double *maxY);

/**
 Retrieving the bounding box from a Raster Object

 \param rst pointer to the Raster Object.
 
 \return a Geometry Object (rectangle) corresponding to the Raster's BBox; 
	NULL if any error is encountered or if the Raster doesn't support any GeoReferencing.

 \sa rl2_create_raster
 */
    RL2_DECLARE gaiaGeomCollPtr rl2_get_raster_bbox (rl2RasterPtr rst);

/**
 Creates a new Pixel Object suitable for a given Raster Object

 \param rst pointer to the Raster Object.
 
 \return pointer to the newly created Pixel Object; NULL if any error is encountered.

 \sa rl2_create_raster, rl2_create_pixel, rl2_destroy_pixel
 
 \note you are responsible to destroy (before or after) the allocated 
 Pixel object.
 */
    RL2_DECLARE rl2PixelPtr rl2_create_raster_pixel (rl2RasterPtr rst);

/**
 Georeferencing a Raster Object (by specifying its Center Point)

 \param rst pointer to the Raster Object.
 \param srid the SRID value
 \param horz_res horizontal pixel resolution: the size (measured
		in map units) corresponding to a single Pixel.
 \param vert_res vertical pixel resolution.
 \param cx map X coordinate corresponding to the Raster's center point.
 \param cy map Y coordinate corresponding to the Raster's center point.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_georeference_upper_left,
 rl2_raster_georeference_lower_left, rl2_raster_georeference_upper_right,
 rl2_raster_georeference_lower_right, rl2_raster_georeference_frame
 */
    RL2_DECLARE int
	rl2_raster_georeference_center (rl2RasterPtr rst, int srid,
					double horz_res, double vert_res,
					double cx, double cy);

/**
 Georeferencing a Raster Object (by specifying its Upper-Left Corner)

 \param rst pointer to the Raster Object.
 \param srid the SRID value
 \param horz_res horizontal pixel resolution: the size (measured
		in map units) corresponding to a single Pixel.
 \param vert_res vertical pixel resolution.
 \param x map X coordinate corresponding to the Raster's upper-left corner.
 \param y map Y coordinate corresponding to the Raster's upper-left corner.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_georeference_center, 
 rl2_raster_georeference_lower_left, rl2_raster_georeference_upper_right,
 rl2_raster_georeference_lower_right, rl2_raster_georeference_frame
 */
    RL2_DECLARE int
	rl2_raster_georeference_upper_left (rl2RasterPtr rst, int srid,
					    double horz_res, double vert_res,
					    double x, double y);


/**
 Georeferencing a Raster Object (by specifying its Lower-Left Corner)

 \param rst pointer to the Raster Object.
 \param srid the SRID value
 \param horz_res horizontal pixel resolution: the size (measured
		in map units) corresponding to a single Pixel.
 \param vert_res vertical pixel resolution.
 \param x map X coordinate corresponding to the Raster's lower-left corner.
 \param y map Y coordinate corresponding to the Raster's lower-left corner.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_georeference_center, 
 rl2_raster_georeference_upper_left, rl2_raster_georeference_upper_right,
 rl2_raster_georeference_lower_right, rl2_raster_georeference_frame
 */
    RL2_DECLARE int
	rl2_raster_georeference_lower_left (rl2RasterPtr rst, int srid,
					    double horz_res, double vert_res,
					    double x, double y);


/**
 Georeferencing a Raster Object (by specifying its Upper-Right Corner)

 \param rst pointer to the Raster Object.
 \param srid the SRID value
 \param horz_res horizontal pixel resolution: the size (measured
		in map units) corresponding to a single Pixel.
 \param vert_res vertical pixel resolution.
 \param x map X coordinate corresponding to the Raster's upper-right corner.
 \param y map Y coordinate corresponding to the Raster's upper-right corner.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_georeference_center, rl2_raster_georeference_upper_left,
 rl2_raster_georeference_lower_left, rl2_raster_georeference_lower_right, 
 rl2_raster_georeference_frame
 */
    RL2_DECLARE int
	rl2_raster_georeference_upper_right (rl2RasterPtr rst, int srid,
					     double horz_res, double vert_res,
					     double x, double y);

/**
 Georeferencing a Raster Object (by specifying its Lower-Right Corner)

 \param rst pointer to the Raster Object.
 \param srid the SRID value
 \param horz_res horizontal pixel resolution: the size (measured
		in map units) corresponding to a single Pixel.
 \param vert_res vertical pixel resolution.
 \param x map X coordinate corresponding to the Raster's lower-right corner.
 \param y map Y coordinate corresponding to the Raster's lower-right corner.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_georeference_center, rl2_raster_georeference_upper_left,
 rl2_raster_georeference_upper_right, rl2_raster_georeference_lower_left, 
 rl2_raster_georeference_frame
 */
    RL2_DECLARE int
	rl2_raster_georeference_lower_right (rl2RasterPtr rst, int srid,
					     double horz_res, double vert_res,
					     double x, double y);

/**
 Georeferencing a Raster Object (by specifying its Corners)

 \param rst pointer to the Raster Object.
 \param srid the SRID value
 \param min_x map X coordinate corresponding to the Raster's lower-left corner.
 \param min_y map Y coordinate corresponding to the Raster's lower-letf corner.
 \param max_x map X coordinate corresponding to the Raster's upper-right corner.
 \param max_y map Y coordinate corresponding to the Raster's upper-right corner.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_georeference_center, rl2_raster_georeference_upper_left,
 rl2_raster_georeference_upper_right, rl2_raster_georeference_lower_left, 
 rl2_raster_georeference_lower_right
 */
    RL2_DECLARE int
	rl2_raster_georeference_frame (rl2RasterPtr rst, int srid, double min_x,
				       double min_y, double max_x,
				       double max_y);

/**
 Retrieves a Pixel from a Raster Object

 \param rst pointer to the Raster Object.
 \param pxl pointer to an already allocated Pixel Object receiving the pixel's 
	values from the raster.
 \param row pixel's row number. 
 \param col pixel's column numer: pixels are structured as a rectangular array.
 Pixel [0,0] is always positioned on the raster's left-upper corner. 
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_create_pixel, rl2_get_raster_width, 
	rl2_get_raster_height, rl2_set_raster_pixel
 */
    RL2_DECLARE int
	rl2_get_raster_pixel (rl2RasterPtr rst, rl2PixelPtr pxl,
			      unsigned short row, unsigned short col);

/**
 Retrieves a Pixel from a Raster Object

 \param rst pointer to the Raster Object.
 \param pxl pointer to a Pixel Object containing the pixel's 
	values to be stored within the raster.
 \param row pixel's row number. 
 \param col pixel's column numer: pixels are structured as a rectangular array.
 Pixel [0,0] is always positioned on the raster's left-upper corner. 
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_create_pixel, rl2_get_raster_width, 
	rl2_get_raster_height, rl2_get_raster_pixel
 */
    RL2_DECLARE int
	rl2_set_raster_pixel (rl2RasterPtr rst, rl2PixelPtr pxl,
			      unsigned short row, unsigned short col);

/**
 Encodes a Raster Object into the corresponding BLOB serialized format

 \param rst pointer to the Raster Object.
 \param compression one of RL2_COMPRESSION_NONE, RL2_COMPRESSION_DEFLATE,
		RL2_COMPRESSION_LZMA, RL2_COMPRESSION_GIF, RL2_COMPRESSION_PNG,
		RL2_COMPRESSION_JPEG, RL2_COMPRESSION_LOSSY_WEBP or
 		RL2_COMPRESSION_LOSSLESS_WEBP
 \param blob_odd on completion will point to the created encoded BLOB ("odd" half).
 \param blob_odd_sz on completion the variable referenced by this
 pointer will contain the size (in bytes) of the "odd" BLOB.
 \param blob_even on completion will point to the created encoded BLOB ("even" half).
 \param blob_even_sz on completion the variable referenced by this
 pointer will contain the size (in bytes) of the "even" BLOB.
 \param quality compression quality factor (0-100); only meaningfull for
		JPEG or WEBP lossy compressions, ignored in any other case.
 \param litte_endian (boolean) accordingly to required BLOB endianness.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_decode
 
 \note both "odd" and "even" BLOB objects corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocations in
 the most appropriate way.
 
 \note some specific encoding doen't require the "even" BLOB; so be prepared
 to get a NULL pointer in this case.
 */
    RL2_DECLARE int
	rl2_raster_encode (rl2RasterPtr rst, int compression,
			   unsigned char **blob_odd, int *blob_odd_sz,
			   unsigned char **blob_even, int *blob_even_sz,
			   int quality, int little_endian);

/**
 Decodes a Raster Object from the corresponding BLOB serialized format

 \param blob_odd pointer to the encoded BLOB ("odd" half).
 \param blob_odd_sz size (in bytes) of the "odd" BLOB.
 \param blob_even pointer to the encoded BLOB ("even" half): could be NULL.
 \param blob_even_sz size (in bytes) of the "even" BLOB: could be ZERO.
 \param palette pointer to a Palette object (could be NULL, but it could
 be strictly required by Palette-based tiles)
 
 \return the pointer to newly created Raster Object: NULL on failure.

 \sa rl2_create_raster, rl2_raster_encode
 
 \note you are responsible to destroy (before or after) any allocated 
 Raster object.
 
 \note some specific encoding doen't require the "even" BLOB; in this case
 you should set blob_even to NULL and blob_even_sz to ZERO.
 */
    RL2_DECLARE rl2RasterPtr
	rl2_raster_decode (int scale, const unsigned char *blob_odd,
			   int blob_odd_sz, const unsigned char *blob_even,
			   int blob_even_sz, rl2PalettePtr palette);

/**
 Creates an RGB pixel array from a Raster

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_RGBA, rl2_raster_data_to_ARGB,
	rl2_raster_data_to_BGR,	rl2_raster_data_to_BGRA
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Pixel components are always organized as RGBRGBRGB....RGB
 */
    RL2_DECLARE int
	rl2_raster_data_to_RGB (rl2RasterPtr rst, unsigned char **buffer,
				int *buf_size);

/**
 Creates an RGBA pixel array from a Raster

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_RGB, rl2_raster_data_to_ARGB,
	rl2_raster_data_to_BGR,	rl2_raster_data_to_BGRA
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Pixel components are always organized as RGBARGBARGBA....RGBA
 */
    RL2_DECLARE int
	rl2_raster_data_to_RGBA (rl2RasterPtr rst, unsigned char **buffer,
				 int *buf_size);

/**
 Creates an ARGB pixel array from a Raster

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_RGB, rl2_raster_data_to_RGBA,
	rl2_raster_data_to_BGR, rl2_raster_data_to_BGRA
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Pixel components are always organized as ARGBARGBARGB....ARGB
 */
    RL2_DECLARE int
	rl2_raster_data_to_ARGB (rl2RasterPtr rst, unsigned char **buffer,
				 int *buf_size);

/**
 Creates an BGR pixel array from a Raster

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_RGB, rl2_raster_data_to_RGBA,
	rl2_raster_data_to_ARGB, rl2_raster_data_to_BGRA
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Pixel components are always organized as BGRBGRBGR...BGR
 */
    RL2_DECLARE int
	rl2_raster_data_to_BGR (rl2RasterPtr rst, unsigned char **buffer,
				int *buf_size);

/**
 Creates an BGRA pixel array from a Raster

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_RGB, rl2_raster_data_to_RGBA,
	rl2_raster_data_to_ARGB, rl2_raster_data_to_BGR
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Pixel components are always organized as BGRABGRABGRA...BGRA
 */
    RL2_DECLARE int
	rl2_raster_data_to_BGRA (rl2RasterPtr rst, unsigned char **buffer,
				 int *buf_size);

/**
 Creates a pixel array from a Raster (1 bit per sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_2bit, rl2_raster_data_to_4bit,
	rl2_raster_data_to_int8, rl2_raster_data_to_uint8, rl2_raster_data_to_int16,
	rl2_raster_data_to_uint16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 */
    RL2_DECLARE int
	rl2_raster_data_to_1bit (rl2RasterPtr rst, unsigned char **buffer,
				 int *buf_size);

/**
 Creates a pixel array from a Raster (2 bit per sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_4bit,
	rl2_raster_data_to_int8, rl2_raster_data_to_uint8, rl2_raster_data_to_int16,
	rl2_raster_data_to_uint16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 */
    RL2_DECLARE int
	rl2_raster_data_to_2bit (rl2RasterPtr rst, unsigned char **buffer,
				 int *buf_size);

/**
 Creates a pixel array from a Raster (4 bit per sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_int8, rl2_raster_data_to_uint8, rl2_raster_data_to_int16,
	rl2_raster_data_to_uint16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 */
    RL2_DECLARE int
	rl2_raster_data_to_4bit (rl2RasterPtr rst, unsigned char **buffer,
				 int *buf_size);

/**
 Creates a pixel array from a Raster (Integer, 8 bit sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_uint8, rl2_raster_data_to_int16,
	rl2_raster_data_to_uint16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_int8 (rl2RasterPtr rst, char **buffer,
				 int *buf_size);

/**
 Creates a pixel array from a Raster (unsigned Integer, 8 bit sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_int8, rl2_raster_data_to_int16,
	rl2_raster_data_to_uint16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double, rl2_raster_band_to_uint8
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_uint8 (rl2RasterPtr rst, unsigned char **buffer,
				  int *buf_size);

/**
 Creates a pixel array from a Raster (Integer, 16 bit sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_int8, rl2_raster_data_to_uint8,
	rl2_raster_data_to_uint16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_int16 (rl2RasterPtr rst, short **buffer,
				  int *buf_size);

/**
 Creates a pixel array from a Raster (unsigned Integer, 16 bit sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_int8, rl2_raster_data_to_uint8,
	rl2_raster_data_to_int16, rl2_raster_data_to_int32, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double, rl2_raster_band_to_uint16
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_uint16 (rl2RasterPtr rst, unsigned short **buffer,
				   int *buf_size);

/**
 Creates a pixel array from a Raster (Integer, 32 bit sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_int8, rl2_raster_data_to_uint8,
	rl2_raster_data_to_int16, rl2_raster_data_to_uint16, rl2_raster_data_to_uint32,
	rl2_raster_data_to_float, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_int32 (rl2RasterPtr rst, int **buffer,
				  int *buf_size);

/**
 Creates a pixel array from a Raster (unsigned Integer, 32 bit sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_int8, rl2_raster_data_to_uint8,
	rl2_raster_data_to_int16, rl2_raster_data_to_uint16, rl2_raster_data_to_int32,
	rl2_raster_data_to_float, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_uint32 (rl2RasterPtr rst, unsigned int **buffer,
				   int *buf_size);

/**
 Creates a pixel array from a Raster (floating point, single precision sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_int8, rl2_raster_data_to_uint8,
	rl2_raster_data_to_int16, rl2_raster_data_to_uint16, rl2_raster_data_to_int32,
	rl2_raster_data_to_uint32, rl2_raster_data_to_double
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_float (rl2RasterPtr rst, float **buffer,
				  int *buf_size);

/**
 Creates a pixel array from a Raster (floating point, double precision sample)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_1bit, rl2_raster_data_to_2bit,
	rl2_raster_data_to_4bit, rl2_raster_data_to_int8, rl2_raster_data_to_uint8,
	rl2_raster_data_to_int16, rl2_raster_data_to_uint16, rl2_raster_data_to_int32,
	rl2_raster_data_to_uint32, rl2_raster_data_to_float
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_data_to_double (rl2RasterPtr rst, double **buffer,
				   int *buf_size);

/**
 Creates a pixel array from a Multiband Raster (unsigned Integer, 8 bit sample, single Band)

 \param rst pointer to the Raster Object.
 \param band the selected Sample/Band index (the first sample corresponds to index ZERO).
 \param buffer on completion will point to the created pixel/Band array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel/Band array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_uint8, rl2_raster_band_to_uint16,
	rl2_raster_bands_to_RGB
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_band_to_uint8 (rl2RasterPtr rst, int band,
				  unsigned char **buffer, int *buf_size);

/**
 Creates a pixel array from a Multiband Raster (unsigned Integer, 16 bit sample, single Band)

 \param rst pointer to the Raster Object.
 \param band the selected Sample/Band index (the first sample corresponds to index ZERO).
 \param buffer on completion will point to the created pixel/Band array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel/Band array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_uint16, rl2_raster_band_to_uint8,
	rl2_raster_bands_to_RGB
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Endianness will always correspond to CPU natural architecture.
 */
    RL2_DECLARE int
	rl2_raster_band_to_uint16 (rl2RasterPtr rst, int band,
				   unsigned short **buffer, int *buf_size);

/**
 Creates an RGB pixel array from a Multiband Raster

 \param rst pointer to the Raster Object.
 \param bandR an arbitrary Sample/Band index assumed to represent the Red Band
 (the first sample corresponds to index ZERO).
 \param bandG an arbitrary Sample/Band index assumed to represent the Green Band.
 \param bandB an arbitrary Sample/Band index assumed to represent the Blue Band.
 \param buffer on completion will point to the created pixel array.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the pixel array.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster, rl2_raster_data_to_uint8, rl2_raster_band_to_uint8,
  
 \note the arrays returned by this function corresponds to dynamic memory;
 you are responsible for properly freeing such dynamic allocation in
 the most appropriate way.
 
 \note pixels into the returned array will be tightly packed and will be
 organized by increasing rows and columns; the first pixel will correspond
 to Row=0,Column=0, the second pixel to Row=0,Column=1 and so on.
 Pixel components are always organized as RGBRGBRGB....RGB
 */
    RL2_DECLARE int
	rl2_raster_bands_to_RGB (rl2RasterPtr rst, int bandR, int bandG,
				 int bandB, unsigned char **buffer,
				 int *buf_size);

/**
 Exports a Raster object as an in-memory stored GIF image

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the memory block storing the created GIF image.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the GIF image.
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_raster, rl2_raster_from_gif
 */
    RL2_DECLARE int
	rl2_raster_to_gif (rl2RasterPtr rst, unsigned char **gif,
			   int *gif_size);

/**
 Allocates and initializes a new Raster object from an in-memory stored GIF image

 \param gif pointer to the memory block storing the input GIF image.
 \param gif_size size (in bytes) of the GIF image.
 
 \return the pointer to newly created Raster Object: NULL on failure.
 
 \sa rl2_destroy_raster, rl2_create_raster, rl2_raster_to_gif
 
 \note you are responsible to destroy (before or after) any allocated 
 Raster object.
 */
    RL2_DECLARE rl2RasterPtr rl2_raster_from_gif (unsigned char *gif,
						  int gif_size);

/**
 Exports a Raster object as an in-memory stored PNG image

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the memory block storing the created PNG image.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the PNG image.
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_raster, rl2_raster_from_png
 */
    RL2_DECLARE int
	rl2_raster_to_png (rl2RasterPtr rst, unsigned char **png,
			   int *png_size);

/**
 Allocates and initializes a new Raster object from an in-memory stored PNG image

 \param png pointer to the memory block storing the input PNG image.
 \param png_size size (in bytes) of the PNG image.
 
 \return the pointer to newly created Raster Object: NULL on failure.
 
 \sa rl2_destroy_raster, rl2_create_raster, rl2_raster_to_png
 
 \note you are responsible to destroy (before or after) any allocated 
 Raster object.
 */
    RL2_DECLARE rl2RasterPtr rl2_raster_from_png (unsigned char *png,
						  int png_size);

/**
 Exports a Raster object as an in-memory stored JPEG image

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the memory block storing the created JPEG image.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the JPEG image.
 \param quality compression quality factor (0-100)
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_raster, rl2_raster_from_jpeg
 */
    RL2_DECLARE int
	rl2_raster_to_jpeg (rl2RasterPtr rst, unsigned char **jpeg,
			    int *jpeg_size, int quality);

/**
 Allocates and initializes a new Raster object from an in-memory stored JPEG image

 \param jpeg pointer to the memory block storing the input JPEG image.
 \param jpeg_size size (in bytes) of the JPEG image.
 
 \return the pointer to newly created Raster Object: NULL on failure.
 
 \sa rl2_destroy_raster, rl2_create_raster, rl2_raster_to_jpeg
 
 \note you are responsible to destroy (before or after) any allocated 
 Raster object.
 */
    RL2_DECLARE rl2RasterPtr
	rl2_raster_from_jpeg (unsigned char *jpeg, int jpeg_size);

/**
 Exports a Raster object as an in-memory stored WEBP image (lossy compressed)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the memory block storing the created WEBP image.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the WEBP image.
 \param quality compression quality factor (0-100)
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_raster, rl2_raster_from_webp, rl2_raster_to_lossless_webp
 */
    RL2_DECLARE int
	rl2_raster_to_lossy_webp (rl2RasterPtr rst, unsigned char **webp,
				  int *webp_size, int quality);

/**
 Exports a Raster object as an in-memory stored WEBP image (lossless compressed)

 \param rst pointer to the Raster Object.
 \param buffer on completion will point to the memory block storing the created WEBP image.
 \param buf_size on completion the variable referenced by this
 pointer will contain the size (in bytes) of the WEBP image.
 
 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa rl2_create_raster, rl2_raster_from_webp, rl2_raster_to_lossy_webp
 */
    RL2_DECLARE int
	rl2_raster_to_lossless_webp (rl2RasterPtr rst, unsigned char **webp,
				     int *webp_size);

/**
 Allocates and initializes a new Raster object from an in-memory stored WEBP image

 \param webp pointer to the memory block storing the input WEBP image.
 \param webp_size size (in bytes) of the WEBP image.
 
 \return the pointer to newly created Raster Object: NULL on failure.
 
 \sa rl2_destroy_raster, rl2_create_raster, rl2_raster_to_lossy_webp,
	rl2_raster_to_lossless_webp
 
 \note you are responsible to destroy (before or after) any allocated 
 Raster object.
 */
    RL2_DECLARE rl2RasterPtr
	rl2_raster_from_webp (unsigned char *webp, int webp_size);

    RL2_DECLARE rl2PalettePtr rl2_get_raster_palette (rl2RasterPtr raster);

    RL2_DECLARE rl2PalettePtr rl2_clone_palette (rl2PalettePtr palette);

    RL2_DECLARE rl2PixelPtr
	rl2_rescale_block (rl2RasterPtr rst, int row, int col, int size);

    RL2_DECLARE rl2CoveragePtr
	rl2_create_coverage_from_dbms (sqlite3 * handle, const char *coverage);

    RL2_DECLARE int
	rl2_find_matching_resolution (sqlite3 * handle, const char *coverage,
				      double *x_res, double *y_res,
				      unsigned char *level,
				      unsigned char *scale);

    RL2_DECLARE int
	rl2_get_raw_raster_data (sqlite3 * handle, rl2CoveragePtr cvg,
				 unsigned short width, unsigned short height,
				 double minx, double miny, double maxx,
				 double maxy, double x_res, double y_res,
				 unsigned char **buffer, int *buf_size,
				 rl2PalettePtr * palette);

    RL2_DECLARE int
	rl2_create_dbms_coverage (sqlite3 * handle, const char *coverage,
				  unsigned char sample, unsigned char pixel,
				  unsigned char num_bands,
				  unsigned char compression, int quality,
				  unsigned short tile_width,
				  unsigned short tile_height, int srid,
				  double x_res, double y_res);

    RL2_DECLARE int
	rl2_delete_dbms_section (sqlite3 * handle, const char *coverage,
				 sqlite3_int64 section_id);

    RL2_DECLARE int
	rl2_get_dbms_section_id (sqlite3 * handle, const char *coverage,
				 const char *section,
				 sqlite3_int64 * section_id);

    RL2_DECLARE int
	rl2_drop_dbms_coverage (sqlite3 * handle, const char *coverage);

    RL2_DECLARE int
	rl2_create_default_dbms_palette (unsigned char **blob, int *blob_size);

    RL2_DECLARE int
	rl2_serialize_dbms_palette (rl2PalettePtr palette, unsigned char **blob,
				    int *blob_size);

    RL2_DECLARE rl2PalettePtr
	rl2_deserialize_dbms_palette (const unsigned char *blob, int blob_size);

    RL2_DECLARE rl2PalettePtr
	rl2_get_dbms_palette (sqlite3 * handle, const char *coverage);

    RL2_DECLARE int
	rl2_update_dbms_palette (sqlite3 * handle, const char *coverage,
				 rl2PalettePtr palette);

    RL2_DECLARE int
	rl2_check_dbms_palette (sqlite3 * handle, rl2CoveragePtr cvg,
				rl2TiffOriginPtr tiff);

#ifdef __cplusplus
}
#endif

#endif				/* _RASTERLITE2_H */
