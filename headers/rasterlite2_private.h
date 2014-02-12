/*

 rasterlite2_private -- hidden internals 

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
 \file rasterlite2_private.h

 RasterLite2 private header file
 */

/*
/ the following patch supporting GeoTiff headers
/ was kindly contributed by Brad Hards on 2011-09-02
/ for libgaigraphics (the ancestor of librasterlite2)
*/
#ifdef HAVE_GEOTIFF_GEOTIFF_H
#include <geotiff/geotiff.h>
#include <geotiff/xtiffio.h>
#include <geotiff/geo_tiffp.h>
#include <geotiff/geo_keyp.h>
#include <geotiff/geovalues.h>
#include <geotiff/geo_normalize.h>
#elif HAVE_LIBGEOTIFF_GEOTIFF_H
#include <libgeotiff/geotiff.h>
#include <libgeotiff/xtiffio.h>
#include <libgeotiff/geo_tiffp.h>
#include <libgeotiff/geo_keyp.h>
#include <libgeotiff/geovalues.h>
#include <libgeotiff/geo_normalize.h>
#else
#include <geotiff.h>
#include <xtiffio.h>
#include <geo_tiffp.h>
#include <geo_keyp.h>
#include <geovalues.h>
#include <geo_normalize.h>
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _WIN32
#ifdef DLL_EXPORT
#define RL2_PRIVATE
#else
#define RL2_PRIVATE
#endif
#else
#define RL2_PRIVATE __attribute__ ((visibility("hidden")))
#endif
#endif

#ifndef _RASTERLITE2_PRIVATE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RASTERLITE2_PRIVATE_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* internal binary format markers */
#define RL2_ODD_BLOCK_START			0xfa
#define RL2_ODD_BLOCK_END			0xf0
#define RL2_EVEN_BLOCK_START			0xdb
#define RL2_EVEN_BLOCK_END			0xd0
#define RL2_LITTLE_ENDIAN			0x01
#define RL2_BIG_ENDIAN				0x00
#define RL2_PALETTE_START			0xa4
#define RL2_PALETTE_END				0xa5
#define RL2_DATA_START				0xc8
#define RL2_DATA_END				0xc9
#define RL2_MASK_START				0xb6
#define RL2_MASK_END				0xb7
#define RL2_STATS_START				0x27
#define RL2_STATS_END				0x2a
#define RL2_BAND_STATS_START			0x37
#define RL2_BAND_STATS_END			0x3a
#define RL2_HISTOGRAM_START			0x47
#define RL2_HISTOGRAM_END			0x4a
#define RL2_NO_DATA_START			0x03
#define RL2_NO_DATA_END				0x23
#define RL2_SAMPLE_START			0x06
#define RL2_SAMPLE_END				0x26

/* internal ColorSpace forced conversions */
#define RL2_CONVERT_NO				0x00
#define RL2_CONVERT_MONOCHROME_TO_PALETTE	0x01
#define RL2_CONVERT_PALETTE_TO_MONOCHROME	0x02
#define RL2_CONVERT_RGB_TO_GRAYSCALE 		0x03
#define RL2_CONVERT_GRAYSCALE_TO_RGB 		0x04
#define RL2_CONVERT_PALETTE_TO_GRAYSCALE	0x05
#define RL2_CONVERT_GRAYSCALE_TO_PALETTE	0x06
#define RL2_CONVERT_PALETTE_TO_RGB		0x07
#define RL2_CONVERT_GRID_INT8_TO_UINT8		0x08
#define RL2_CONVERT_GRID_INT8_TO_INT16		0x09
#define RL2_CONVERT_GRID_INT8_TO_UINT16		0x0a
#define RL2_CONVERT_GRID_INT8_TO_INT32		0x0b
#define RL2_CONVERT_GRID_INT8_TO_UINT32		0x0c
#define RL2_CONVERT_GRID_INT8_TO_FLOAT		0x0d
#define RL2_CONVERT_GRID_INT8_TO_DOUBLE		0x0e
#define RL2_CONVERT_GRID_INT16_TO_INT8		0x0f
#define RL2_CONVERT_GRID_INT16_TO_UINT8		0x10
#define RL2_CONVERT_GRID_INT16_TO_UINT16	0x11
#define RL2_CONVERT_GRID_INT16_TO_INT32		0x12
#define RL2_CONVERT_GRID_INT16_TO_UINT32	0x13
#define RL2_CONVERT_GRID_INT16_TO_FLOAT		0x14
#define RL2_CONVERT_GRID_INT16_TO_DOUBLE	0x15
#define RL2_CONVERT_GRID_INT32_TO_UINT8		0x16
#define RL2_CONVERT_GRID_INT32_TO_INT8		0x17
#define RL2_CONVERT_GRID_INT32_TO_UINT16	0x18
#define RL2_CONVERT_GRID_INT32_TO_INT16		0x19
#define RL2_CONVERT_GRID_INT32_TO_UINT32	0x1a
#define RL2_CONVERT_GRID_INT32_TO_FLOAT		0x1b
#define RL2_CONVERT_GRID_INT32_TO_DOUBLE	0x1c
#define RL2_CONVERT_GRID_UINT8_TO_INT8		0x1d
#define RL2_CONVERT_GRID_UINT8_TO_INT16		0x1e
#define RL2_CONVERT_GRID_UINT8_TO_UINT16	0x1f
#define RL2_CONVERT_GRID_UINT8_TO_INT32		0x20
#define RL2_CONVERT_GRID_UINT8_TO_UINT32	0x21
#define RL2_CONVERT_GRID_UINT8_TO_FLOAT		0x22
#define RL2_CONVERT_GRID_UINT8_TO_DOUBLE	0x23
#define RL2_CONVERT_GRID_UINT16_TO_INT8		0x24
#define RL2_CONVERT_GRID_UINT16_TO_UINT8	0x25
#define RL2_CONVERT_GRID_UINT16_TO_INT16	0x26
#define RL2_CONVERT_GRID_UINT16_TO_INT32	0x27
#define RL2_CONVERT_GRID_UINT16_TO_UINT32	0x28
#define RL2_CONVERT_GRID_UINT16_TO_FLOAT	0x29
#define RL2_CONVERT_GRID_UINT16_TO_DOUBLE	0x2a
#define RL2_CONVERT_GRID_UINT32_TO_UINT8	0x2b
#define RL2_CONVERT_GRID_UINT32_TO_INT8		0x2c
#define RL2_CONVERT_GRID_UINT32_TO_UINT16	0x2d
#define RL2_CONVERT_GRID_UINT32_TO_INT16	0x2e
#define RL2_CONVERT_GRID_UINT32_TO_INT32	0x2f
#define RL2_CONVERT_GRID_UINT32_TO_FLOAT	0x30
#define RL2_CONVERT_GRID_UINT32_TO_DOUBLE	0x31
#define RL2_CONVERT_GRID_FLOAT_TO_UINT8		0x32
#define RL2_CONVERT_GRID_FLOAT_TO_INT8		0x33
#define RL2_CONVERT_GRID_FLOAT_TO_UINT16	0x34
#define RL2_CONVERT_GRID_FLOAT_TO_INT16		0x35
#define RL2_CONVERT_GRID_FLOAT_TO_UINT32	0x36
#define RL2_CONVERT_GRID_FLOAT_TO_INT32		0x37
#define RL2_CONVERT_GRID_FLOAT_TO_DOUBLE	0x38
#define RL2_CONVERT_GRID_DOUBLE_TO_UINT8	0x39
#define RL2_CONVERT_GRID_DOUBLE_TO_INT8		0x3a
#define RL2_CONVERT_GRID_DOUBLE_TO_UINT16	0x3b
#define RL2_CONVERT_GRID_DOUBLE_TO_INT16	0x3c
#define RL2_CONVERT_GRID_DOUBLE_TO_UINT32	0x3d
#define RL2_CONVERT_GRID_DOUBLE_TO_INT32	0x3e
#define RL2_CONVERT_GRID_DOUBLE_TO_FLOAT	0x3f

    typedef union rl2_priv_sample
    {
	char int8;
	unsigned char uint8;
	short int16;
	unsigned short uint16;
	int int32;
	unsigned int uint32;
	float float32;
	double float64;
    } rl2PrivSample;
    typedef rl2PrivSample *rl2PrivSamplePtr;

    typedef struct rl2_priv_pixel
    {
	unsigned char sampleType;
	unsigned char pixelType;
	unsigned char nBands;
	unsigned char isTransparent;
	rl2PrivSamplePtr Samples;
    } rl2PrivPixel;
    typedef rl2PrivPixel *rl2PrivPixelPtr;

    typedef struct rl2_priv_palette_entry
    {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
    } rl2PrivPaletteEntry;
    typedef rl2PrivPaletteEntry *rl2PrivPaletteEntryPtr;

    typedef struct rl2_priv_palette
    {
	unsigned short nEntries;
	rl2PrivPaletteEntryPtr entries;
    } rl2PrivPalette;
    typedef rl2PrivPalette *rl2PrivPalettePtr;

    typedef struct rl2_priv_raster
    {
	unsigned char sampleType;
	unsigned char pixelType;
	unsigned char nBands;
	unsigned short width;
	unsigned short height;
	double minX;
	double minY;
	double maxX;
	double maxY;
	int Srid;
	double hResolution;
	double vResolution;
	unsigned char *rasterBuffer;
	unsigned char *maskBuffer;
	rl2PrivPalettePtr Palette;
	rl2PrivPixelPtr noData;
    } rl2PrivRaster;
    typedef rl2PrivRaster *rl2PrivRasterPtr;

    typedef struct rl2_priv_tile
    {
	unsigned short pyramidLevel;
	unsigned short sectionRow;
	unsigned short sectionColumn;
	rl2PrivRasterPtr Raster;
    } rl2PrivTile;
    typedef rl2PrivTile *rl2PrivTilePtr;

    typedef struct rl2_priv_section
    {
	char *sectionName;
	unsigned char Compression;
	unsigned short tileWidth;
	unsigned short tileHeight;
	rl2PrivRasterPtr Raster;
    } rl2PrivSection;
    typedef rl2PrivSection *rl2PrivSectionPtr;

    typedef struct rl2_priv_coverage
    {
	char *coverageName;
	unsigned char sampleType;
	unsigned char pixelType;
	unsigned char nBands;
	unsigned char Compression;
	int Quality;
	unsigned short tileWidth;
	unsigned short tileHeight;
	int Srid;
	double hResolution;
	double vResolution;
	rl2PrivPixelPtr noData;
    } rl2PrivCoverage;
    typedef rl2PrivCoverage *rl2PrivCoveragePtr;

    typedef struct rl2_priv_tiff_origin
    {
	char *path;
	char *tfw_path;
	int isGeoTiff;
	TIFF *in;
	int isTiled;
	uint32 width;
	uint32 height;
	uint32 tileWidth;
	uint32 tileHeight;
	uint32 rowsPerStrip;
	uint16 bitsPerSample;
	uint16 samplesPerPixel;
	uint16 photometric;
	uint16 compression;
	uint16 sampleFormat;
	uint16 planarConfig;
	unsigned short maxPalette;
	unsigned char *red;
	unsigned char *green;
	unsigned char *blue;
	unsigned char *alpha;
	unsigned short remapMaxPalette;
	unsigned char *remapRed;
	unsigned char *remapGreen;
	unsigned char *remapBlue;
	unsigned char *remapAlpha;
	int isGeoReferenced;
	int Srid;
	double hResolution;
	double vResolution;
	char *srsName;
	char *proj4text;
	double minX;
	double minY;
	double maxX;
	double maxY;
	unsigned char forced_sample_type;
	unsigned char forced_pixel_type;
	unsigned char forced_num_bands;
	unsigned char forced_conversion;
    } rl2PrivTiffOrigin;
    typedef rl2PrivTiffOrigin *rl2PrivTiffOriginPtr;

    typedef struct rl2_priv_tiff_destination
    {
	char *path;
	char *tfw_path;
	int isGeoTiff;
	TIFF *out;
	GTIF *gtif;
	void *tiffBuffer;
	uint32 width;
	uint32 height;
	int isTiled;
	uint32 tileWidth;
	uint32 tileHeight;
	uint32 rowsPerStrip;
	uint16 bitsPerSample;
	uint16 samplesPerPixel;
	uint16 photometric;
	uint16 compression;
	uint16 sampleFormat;
	unsigned short maxPalette;
	unsigned char *red;
	unsigned char *green;
	unsigned char *blue;
	int isGeoReferenced;
	int Srid;
	double hResolution;
	double vResolution;
	char *srsName;
	char *proj4text;
	double minX;
	double minY;
	double maxX;
	double maxY;
    } rl2PrivTiffDestination;
    typedef rl2PrivTiffDestination *rl2PrivTiffDestinationPtr;

    typedef struct rl2_priv_ascii_origin
    {
	char *path;
	FILE *tmp;
	unsigned short width;
	unsigned short height;
	int Srid;
	double hResolution;
	double vResolution;
	double minX;
	double minY;
	double maxX;
	double maxY;
	double noData;
	unsigned char sample_type;
    } rl2PrivAsciiOrigin;
    typedef rl2PrivAsciiOrigin *rl2PrivAsciiOriginPtr;

    typedef struct rl2_priv_ascii_destination
    {
	char *path;
	FILE *out;
	unsigned short width;
	unsigned short height;
	double Resolution;
	double X;
	double Y;
	int isCentered;
	double noData;
	int decimalDigits;
	unsigned short nextLineNo;
	char headerDone;
	void *pixels;
	unsigned char sampleType;
    } rl2PrivAsciiDestination;
    typedef rl2PrivAsciiDestination *rl2PrivAsciiDestinationPtr;

    typedef struct rl2_pool_variance
    {
	double variance;
	double count;
	struct rl2_pool_variance *next;
    } rl2PoolVariance;
    typedef rl2PoolVariance *rl2PoolVariancePtr;

    typedef struct rl2_priv_band_statistics
    {
	double min;
	double max;
	double mean;
	double sum_sq_diff;
	unsigned short nHistogram;
	double *histogram;
	rl2PoolVariancePtr first;
	rl2PoolVariancePtr last;
    } rl2PrivBandStatistics;
    typedef rl2PrivBandStatistics *rl2PrivBandStatisticsPtr;

    typedef struct rl2_priv_raster_statistics
    {
	double no_data;
	double count;
	unsigned char sampleType;
	unsigned char nBands;
	rl2PrivBandStatisticsPtr band_stats;
    } rl2PrivRasterStatistics;
    typedef rl2PrivRasterStatistics *rl2PrivRasterStatisticsPtr;

    typedef struct wms_retry_item
    {
	int done;
	int count;
	double minx;
	double miny;
	double maxx;
	double maxy;
	struct wms_retry_item *next;
    } WmsRetryItem;
    typedef WmsRetryItem *WmsRetryItemPtr;

    typedef struct wms_retry_list
    {
	WmsRetryItemPtr first;
	WmsRetryItemPtr last;
    } WmsRetryList;
    typedef WmsRetryList *WmsRetryListPtr;

    typedef struct section_pyramid_tile_in
    {
	sqlite3_int64 tile_id;
	double cx;
	double cy;
	struct section_pyramid_tile_in *next;
    } SectionPyramidTileIn;
    typedef SectionPyramidTileIn *SectionPyramidTileInPtr;

    typedef struct section_pyramid_tile_ref
    {
	SectionPyramidTileInPtr child;
	struct section_pyramid_tile_ref *next;
    } SectionPyramidTileRef;
    typedef SectionPyramidTileRef *SectionPyramidTileRefPtr;

    typedef struct section_pyramid_tile_out
    {
	unsigned short row;
	unsigned short col;
	double minx;
	double miny;
	double maxx;
	double maxy;
	SectionPyramidTileRefPtr first;
	SectionPyramidTileRefPtr last;
	struct section_pyramid_tile_out *next;
    } SectionPyramidTileOut;
    typedef SectionPyramidTileOut *SectionPyramidTileOutPtr;

    typedef struct section_pyramid
    {
	sqlite3_int64 section_id;
	int scale;
	unsigned char sample_type;
	unsigned char pixel_type;
	unsigned char num_samples;
	unsigned char compression;
	int quality;
	int srid;
	unsigned short full_width;
	unsigned short full_height;
	double res_x;
	double res_y;
	unsigned short scaled_width;
	unsigned short scaled_height;
	double tile_width;
	double tile_height;
	double minx;
	double miny;
	double maxx;
	double maxy;
	SectionPyramidTileInPtr first_in;
	SectionPyramidTileInPtr last_in;
	SectionPyramidTileOutPtr first_out;
	SectionPyramidTileOutPtr last_out;
    } SectionPyramid;
    typedef SectionPyramid *SectionPyramidPtr;

    RL2_PRIVATE int
	rl2_blob_from_file (const char *path, unsigned char **blob,
			    int *blob_size);

    RL2_PRIVATE int
	rl2_blob_to_file (const char *path, unsigned char *blob, int blob_size);

    RL2_PRIVATE int
	rl2_decode_jpeg_scaled (int scale, const unsigned char *jpeg,
				int jpeg_sz, unsigned short *width,
				unsigned short *height,
				unsigned char *pixel_type,
				unsigned char **pixels, int *pixels_sz);

    RL2_PRIVATE int
	rl2_decode_webp_scaled (int scale, const unsigned char *webp,
				int webp_sz, unsigned short *width,
				unsigned short *height,
				unsigned char pixel_type,
				unsigned char **pixels, int *pixels_sz,
				unsigned char **mask, int *mask_sz);

    RL2_PRIVATE int
	rl2_data_to_png (const unsigned char *pixels, const unsigned char *mask,
			 rl2PalettePtr plt, unsigned short width,
			 unsigned short height, unsigned char sample_type,
			 unsigned char pixel_type, unsigned char **compr_data,
			 int *compressed_size);

    RL2_PRIVATE int
	rl2_decode_png (const unsigned char *png, int png_sz,
			unsigned short *width, unsigned short *height,
			unsigned char *sample_type, unsigned char *pixel_type,
			unsigned char *num_bands, unsigned char **pixels,
			int *pixels_sz, unsigned char **mask, int *mask_sz,
			rl2PalettePtr * palette);

    RL2_PRIVATE int
	rl2_data_to_gif (const unsigned char *pixels, const unsigned char *mask,
			 rl2PalettePtr plt, unsigned short width,
			 unsigned short height, unsigned char sample_type,
			 unsigned char pixel_type, unsigned char **compr_data,
			 int *compressed_size);

    RL2_PRIVATE int
	rl2_decode_gif (const unsigned char *gif, int gif_sz,
			unsigned short *width, unsigned short *height,
			unsigned char *sample_type, unsigned char *pixel_type,
			unsigned char **pixels, int *pixels_sz,
			rl2PalettePtr * palette);

    RL2_PRIVATE int
	rl2_data_to_jpeg (const unsigned char *pixels,
			  const unsigned char *mask, rl2PalettePtr plt,
			  unsigned short width, unsigned short height,
			  unsigned char sample_type, unsigned char pixel_type,
			  unsigned char **jpeg, int *jpeg_size, int quality);

    RL2_PRIVATE int
	rl2_decode_tiff_mono4 (const unsigned char *tiff, int tiff_sz,
			       unsigned short *width, unsigned short *height,
			       unsigned char **pixels, int *pixels_sz);

    RL2_PRIVATE char truncate_8 (double val);

    RL2_PRIVATE unsigned char truncate_u8 (double val);

    RL2_PRIVATE short truncate_16 (double val);

    RL2_PRIVATE unsigned short truncate_u16 (double val);

    RL2_PRIVATE int truncate_32 (double val);

    RL2_PRIVATE unsigned int truncate_u32 (double val);

    RL2_PRIVATE void void_raw_buffer_palette (unsigned char *buffer,
					      unsigned short width,
					      unsigned short height,
					      rl2PalettePtr palette,
					      rl2PixelPtr no_data);

    RL2_PRIVATE void void_raw_buffer (unsigned char *buffer,
				      unsigned short width,
				      unsigned short height,
				      unsigned char sample_type,
				      unsigned char num_bands,
				      rl2PixelPtr no_data);

    RL2_PRIVATE int load_dbms_tiles (sqlite3 * handle,
				     sqlite3_stmt * stmt_tiles,
				     sqlite3_stmt * stmt_data,
				     unsigned char *outbuf,
				     unsigned short width,
				     unsigned short height,
				     unsigned char sample_type,
				     unsigned char num_bands, double x_res,
				     double y_res, double minx, double miny,
				     double maxx, double maxy, int level,
				     int scale, rl2PalettePtr palette,
				     rl2PixelPtr no_data);

    RL2_PRIVATE int load_dbms_tiles_section (sqlite3 * handle,
					     sqlite3_int64 section_id,
					     sqlite3_stmt * stmt_tiles,
					     sqlite3_stmt * stmt_data,
					     unsigned char *outbuf,
					     unsigned short width,
					     unsigned short height,
					     unsigned char sample_type,
					     unsigned char num_bands,
					     double x_res, double y_res,
					     double minx, double maxy,
					     int scale, rl2PalettePtr palette,
					     rl2PixelPtr no_data);

#ifdef __cplusplus
}
#endif

#endif				/* _RASTERLITE2_PRIVATE_H */
