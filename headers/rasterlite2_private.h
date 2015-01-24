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

#include "config.h"

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
#define RL2_CHARLS_START			0x35
#define RL2_CHARLS_END				0x79

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

/* internal Style Rule constants */
#define RL2_UNKNOWN_STYLE				0xf0
#define RL2_VECTOR_STYLE				0xfa
#define RL2_RASTER_STYLE				0xfb

/* internal Graphic types */
#define RL2_UNKNOWN_GRAPHIC		0x8a
#define RL2_EXTERNAL_GRAPHIC	0x8c
#define RL2_MARK_GRAPHIC		0x8d

/* internal Style Rule comparison Ops */
#define RL2_COMPARISON_NONE				0xa0
#define RL2_COMPARISON_EQ				0xa1
#define RL2_COMPARISON_NE				0xa2
#define RL2_COMPARISON_LT				0xa3
#define RL2_COMPARISON_GT				0xa4
#define RL2_COMPARISON_LE				0xa5
#define RL2_COMPARISON_GE				0xa6
#define RL2_COMPARISON_LIKE				0xa7
#define RL2_COMPARISON_NULL				0xa8
#define RL2_COMPARISON_BETWEEN			0xa9

/* internal RasterSymbolizer constants */
#define RL2_BAND_SELECTION_TRIPLE		0xd1
#define RL2_BAND_SELECTION_MONO			0xd2

/* internal TextSymbolizer constants */
#define RL2_MAX_FONT_FAMILIES	16

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
	unsigned int width;
	unsigned int height;
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
	unsigned int tileWidth;
	unsigned int tileHeight;
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
	unsigned int tileWidth;
	unsigned int tileHeight;
	int Srid;
	double hResolution;
	double vResolution;
	rl2PrivPixelPtr noData;
	int strictResolution;
	int mixedResolutions;
	int sectionPaths;
	int sectionMD5;
	int sectionSummary;
    } rl2PrivCoverage;
    typedef rl2PrivCoverage *rl2PrivCoveragePtr;

    typedef struct rl2_priv_vector_layer
    {
	char *f_table_name;
	char *f_geometry_column;
	unsigned short geometry_type;
	int srid;
	unsigned char spatial_index;
    } rl2PrivVectorLayer;
    typedef rl2PrivVectorLayer *rl2PrivVectorLayerPtr;

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
	unsigned short remapMaxPalette;
	unsigned char *remapRed;
	unsigned char *remapGreen;
	unsigned char *remapBlue;
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
	unsigned int width;
	unsigned int height;
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
	unsigned int width;
	unsigned int height;
	double Resolution;
	double X;
	double Y;
	int isCentered;
	double noData;
	int decimalDigits;
	unsigned int nextLineNo;
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

    typedef struct rl2_color_map_ref
    {
	double min;
	double max;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char maxRed;
	unsigned char maxGreen;
	unsigned char maxBlue;
	struct rl2_color_map_ref *next;
    } rl2ColorMapRef;
    typedef rl2ColorMapRef *rl2ColorMapRefPtr;

    typedef struct rl2_color_map_item
    {
	rl2ColorMapRefPtr first;
	rl2ColorMapRefPtr last;
    } rl2ColorMapItem;
    typedef rl2ColorMapItem *rl2ColorMapItemPtr;

    typedef struct rl2_color_map_locator
    {
	int interpolate;
	rl2ColorMapItem look_up[256];
	unsigned char red;
	unsigned char green;
	unsigned char blue;
    } rl2ColorMapLocator;
    typedef rl2ColorMapLocator *rl2ColorMapLocatorPtr;

    typedef struct rl2_band_handling
    {
	unsigned char contrastEnhancement;
	unsigned char look_up[256];
	double minValue;
	double maxValue;
	double scaleFactor;
	rl2ColorMapLocatorPtr colorMap;
    } rl2BandHandling;
    typedef rl2BandHandling *rl2BandHandlingPtr;

    typedef struct rl2_priv_band_selection
    {
	int selectionType;
	unsigned char redBand;
	unsigned char greenBand;
	unsigned char blueBand;
	unsigned char grayBand;
	unsigned char redContrast;
	double redGamma;
	unsigned char greenContrast;
	double greenGamma;
	unsigned char blueContrast;
	double blueGamma;
	unsigned char grayContrast;
	double grayGamma;
    } rl2PrivBandSelection;
    typedef rl2PrivBandSelection *rl2PrivBandSelectionPtr;

    typedef struct rl2_priv_color_map_point
    {
	double value;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	struct rl2_priv_color_map_point *next;
    } rl2PrivColorMapPoint;
    typedef rl2PrivColorMapPoint *rl2PrivColorMapPointPtr;

    typedef struct rl2_priv_color_map_categorize
    {
	unsigned char baseRed;
	unsigned char baseGreen;
	unsigned char baseBlue;
	rl2PrivColorMapPointPtr first;
	rl2PrivColorMapPointPtr last;
	unsigned char dfltRed;
	unsigned char dfltGreen;
	unsigned char dfltBlue;
    } rl2PrivColorMapCategorize;
    typedef rl2PrivColorMapCategorize *rl2PrivColorMapCategorizePtr;

    typedef struct rl2_priv_color_map_interpolate
    {
	rl2PrivColorMapPointPtr first;
	rl2PrivColorMapPointPtr last;
	unsigned char dfltRed;
	unsigned char dfltGreen;
	unsigned char dfltBlue;
    } rl2PrivColorMapInterpolate;
    typedef rl2PrivColorMapInterpolate *rl2PrivColorMapInterpolatePtr;

    typedef struct rl2_priv_raster_symbolizer
    {
	double opacity;
	unsigned char contrastEnhancement;
	double gammaValue;
	rl2PrivBandSelectionPtr bandSelection;
	rl2PrivColorMapCategorizePtr categorize;
	rl2PrivColorMapInterpolatePtr interpolate;
	int shadedRelief;
	int brightnessOnly;
	double reliefFactor;
    } rl2PrivRasterSymbolizer;
    typedef rl2PrivRasterSymbolizer *rl2PrivRasterSymbolizerPtr;

    typedef struct rl2_priv_color_replacement
    {
	int index;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	struct rl2_priv_color_replacement *next;
    } rl2PrivColorReplacement;
    typedef rl2PrivColorReplacement *rl2PrivColorReplacementPtr;

    typedef struct rl2_priv_external_graphic
    {
	char *xlink_href;
	rl2PrivColorReplacementPtr first;
	rl2PrivColorReplacementPtr last;
    } rl2PrivExternalGraphic;
    typedef rl2PrivExternalGraphic *rl2PrivExternalGraphicPtr;

    typedef struct rl2_priv_mark
    {
	unsigned char well_known_type;
	rl2PrivExternalGraphicPtr external_graphic;
	struct rl2_priv_stroke *stroke;
	struct rl2_priv_fill *fill;
    } rl2PrivMark;
    typedef rl2PrivMark *rl2PrivMarkPtr;

    typedef struct rl2_priv_graphic_item
    {
	unsigned char type;
	void *item;
	struct rl2_priv_graphic_item *next;
    } rl2PrivGraphicItem;
    typedef rl2PrivGraphicItem *rl2PrivGraphicItemPtr;

    typedef struct rl2_priv_graphic
    {
	rl2PrivGraphicItemPtr first;
	rl2PrivGraphicItemPtr last;
	double opacity;
	double size;
	double rotation;
	double anchor_point_x;
	double anchor_point_y;
	double displacement_x;
	double displacement_y;
    } rl2PrivGraphic;
    typedef rl2PrivGraphic *rl2PrivGraphicPtr;

    typedef struct rl2_priv_stroke
    {
	rl2PrivGraphicPtr graphic;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	double opacity;
	double width;
	unsigned char linejoin;
	unsigned char linecap;
	int dash_count;
	double *dash_list;
	double dash_offset;
    } rl2PrivStroke;
    typedef rl2PrivStroke *rl2PrivStrokePtr;

    typedef struct rl2_priv_fill
    {
	rl2PrivGraphicPtr graphic;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	double opacity;
    } rl2PrivFill;
    typedef rl2PrivFill *rl2PrivFillPtr;

    typedef struct rl2_priv_point_symbolizer
    {
	rl2PrivGraphicPtr graphic;
    } rl2PrivPointSymbolizer;
    typedef rl2PrivPointSymbolizer *rl2PrivPointSymbolizerPtr;

    typedef struct rl2_priv_line_symbolizer
    {
	rl2PrivStrokePtr stroke;
	double perpendicular_offset;
    } rl2PrivLineSymbolizer;
    typedef rl2PrivLineSymbolizer *rl2PrivLineSymbolizerPtr;

    typedef struct rl2_priv_polygon_symbolizer
    {
	rl2PrivStrokePtr stroke;
	rl2PrivFillPtr fill;
	double displacement_x;
	double displacement_y;
	double perpendicular_offset;
    } rl2PrivPolygonSymbolizer;
    typedef rl2PrivPolygonSymbolizer *rl2PrivPolygonSymbolizerPtr;

    typedef struct rl2_priv_point_placement
    {
	double anchor_point_x;
	double anchor_point_y;
	double displacement_x;
	double displacement_y;
	double rotation;
    } rl2PrivPointPlacement;
    typedef rl2PrivPointPlacement *rl2PrivPointPlacementPtr;

    typedef struct rl2_priv_line_placement
    {
	double perpendicular_offset;
	int is_repeated;
	double initial_gap;
	double gap;
	int is_aligned;
	int generalize_line;
    } rl2PrivLinePlacement;
    typedef rl2PrivLinePlacement *rl2PrivLinePlacementPtr;

    typedef struct rl2_priv_halo
    {
	double radius;
	rl2PrivFillPtr fill;
    } rl2PrivHalo;
    typedef rl2PrivHalo *rl2PrivHaloPtr;

    typedef struct rl2_priv_text_symbolizer
    {
	char *label;
	int font_families_count;
	char *font_families[RL2_MAX_FONT_FAMILIES];
	unsigned char font_style;
	unsigned char font_weight;
	double font_size;
	unsigned char label_placement_type;
	void *label_placement;
	rl2PrivHaloPtr halo;
	rl2PrivFillPtr fill;
    } rl2PrivTextSymbolizer;
    typedef rl2PrivTextSymbolizer *rl2PrivTextSymbolizerPtr;

    typedef struct rl2_priv_vector_symbolizer_item
    {
	unsigned char symbolizer_type;
	void *symbolizer;
	struct rl2_priv_vector_symbolizer_item *next;
    } rl2PrivVectorSymbolizerItem;
    typedef rl2PrivVectorSymbolizerItem *rl2PrivVectorSymbolizerItemPtr;

    typedef struct rl2_priv_vector_symbolizer
    {
	rl2PrivVectorSymbolizerItemPtr first;
	rl2PrivVectorSymbolizerItemPtr last;
    } rl2PrivVectorSymbolizer;
    typedef rl2PrivVectorSymbolizer *rl2PrivVectorSymbolizerPtr;

    typedef struct rl2_priv_rule_single_arg
    {
	char *value;
    } rl2PrivRuleSingleArg;
    typedef rl2PrivRuleSingleArg *rl2PrivRuleSingleArgPtr;

    typedef struct rl2_priv_rule_like_args
    {
	char *wild_card;
	char *single_char;
	char *escape_char;
	char *value;
    } rl2PrivRuleLikeArgs;
    typedef rl2PrivRuleLikeArgs *rl2PrivRuleLikeArgsPtr;

    typedef struct rl2_priv_rule_between_args
    {
	char *lower;
	char *upper;
    } rl2PrivRuleBetweenArgs;
    typedef rl2PrivRuleBetweenArgs *rl2PrivRuleBetweenArgsPtr;

    typedef struct rl2_priv_variant_value
    {
	sqlite3_int64 int_value;
	double dbl_value;
	char *text_value;
	unsigned char *blob_value;
	int bytes;
	int sqlite3_type;
    } rl2PrivVariantValue;
    typedef rl2PrivVariantValue *rl2PrivVariantValuePtr;

    typedef struct rl2_priv_style_rule
    {
	int else_rule;
	double min_scale;
	double max_scale;
	unsigned char comparison_op;
	void *comparison_args;
	char *column_name;
	unsigned char style_type;
	void *style;
	struct rl2_priv_style_rule *next;
    } rl2PrivStyleRule;
    typedef rl2PrivStyleRule *rl2PrivStyleRulePtr;

    typedef struct rl2_priv_coverage_style
    {
	char *name;
	rl2PrivStyleRulePtr first_rule;
	rl2PrivStyleRulePtr last_rule;
    } rl2PrivCoverageStyle;
    typedef rl2PrivCoverageStyle *rl2PrivCoverageStylePtr;

    typedef struct rl2_priv_feature_type_style
    {
	char *name;
	rl2PrivStyleRulePtr first_rule;
	rl2PrivStyleRulePtr last_rule;
	rl2PrivStyleRulePtr else_rule;
    } rl2PrivFeatureTypeStyle;
    typedef rl2PrivFeatureTypeStyle *rl2PrivFeatureTypeStylePtr;

    typedef struct rl2_priv_child_style
    {
	char *namedLayer;
	char *namedStyle;
	int validLayer;
	int validStyle;
	struct rl2_priv_child_style *next;
    } rl2PrivChildStyle;
    typedef rl2PrivChildStyle *rl2PrivChildStylePtr;

    typedef struct rl2_priv_group_style
    {
	char *name;
	rl2PrivChildStylePtr first;
	rl2PrivChildStylePtr last;
	int valid;
    } rl2PrivGroupStyle;
    typedef rl2PrivGroupStyle *rl2PrivGroupStylePtr;

    typedef struct rl2_priv_group_renderer_layer
    {
	int layer_type;
	char *layer_name;
	rl2CoveragePtr coverage;
	sqlite3_int64 raster_style_id;
	rl2PrivRasterSymbolizerPtr raster_symbolizer;
	rl2PrivRasterStatisticsPtr raster_stats;
    } rl2PrivGroupRendererLayer;
    typedef rl2PrivGroupRendererLayer *rl2PrivGroupRendererLayerPtr;

    typedef struct rl2_priv_group_renderer
    {
	int count;
	rl2PrivGroupRendererLayerPtr layers;
    } rl2PrivGroupRenderer;
    typedef rl2PrivGroupRenderer *rl2PrivGroupRendererPtr;

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
	unsigned int row;
	unsigned int col;
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
	unsigned int full_width;
	unsigned int full_height;
	double res_x;
	double res_y;
	unsigned int scaled_width;
	unsigned int scaled_height;
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

    typedef struct resolution_level
    {
	int level;
	int scale;
	int real_scale;
	double x_resolution;
	double y_resolution;
	struct resolution_level *prev;
	struct resolution_level *next;
    } ResolutionLevel;
    typedef ResolutionLevel *ResolutionLevelPtr;

    typedef struct resolutions_list
    {
	ResolutionLevelPtr first;
	ResolutionLevelPtr last;
    } ResolutionsList;
    typedef ResolutionsList *ResolutionsListPtr;

    typedef struct insert_wms
    {
	sqlite3 *sqlite;
	unsigned char *rgba_tile;
	rl2CoveragePtr coverage;
	const char *sect_name;
	int mixedResolutions;
	int sectionPaths;
	int sectionMD5;
	int sectionSummary;
	double x;
	double y;
	int width;
	int height;
	double tilew;
	double tileh;
	int srid;
	double minx;
	double miny;
	double maxx;
	double maxy;
	unsigned char sample_type;
	unsigned char num_bands;
	unsigned char compression;
	double horz_res;
	double vert_res;
	unsigned int tile_width;
	unsigned int tile_height;
	rl2PixelPtr no_data;
	sqlite3_stmt *stmt_sect;
	sqlite3_stmt *stmt_levl;
	sqlite3_stmt *stmt_tils;
	sqlite3_stmt *stmt_data;
	char *xml_summary;
    } InsertWms;
    typedef InsertWms *InsertWmsPtr;

    typedef struct rl2_mem_pdf_target
    {
	unsigned char *buffer;
	int write_offset;
	int size;
    } rl2PrivMemPdf;
    typedef rl2PrivMemPdf *rl2PrivMemPdfPtr;

    struct aux_renderer
    {
	/* helper struct for passing arguments to aux_render_image */
	sqlite3 *sqlite;
	int width;
	int height;
	int base_width;
	int base_height;
	double minx;
	double miny;
	double maxx;
	double maxy;
	int srid;
	int by_section;
	sqlite3_int64 section_id;
	double x_res;
	double y_res;
	double xx_res;
	double yy_res;
	int transparent;
	double opacity;
	int quality;
	unsigned char format_id;
	unsigned char bg_red;
	unsigned char bg_green;
	unsigned char bg_blue;
	rl2CoveragePtr coverage;
	rl2RasterSymbolizerPtr symbolizer;
	rl2RasterStatisticsPtr stats;
	unsigned char *outbuf;
	rl2PalettePtr palette;
	unsigned char out_pixel;
    };

    struct aux_group_renderer
    {
	/* helper struct for passing arguments to aux_group_renderer */
	sqlite3_context *context;
	const char *group_name;
	double minx;
	double maxx;
	double miny;
	double maxy;
	int width;
	int height;
	const char *style;
	unsigned char format_id;
	unsigned char bg_red;
	unsigned char bg_green;
	unsigned char bg_blue;
	int transparent;
	int quality;
	int reaspect;
    };

    RL2_PRIVATE int
	rl2_blob_from_file (const char *path, unsigned char **blob,
			    int *blob_size);

    RL2_PRIVATE int
	rl2_blob_to_file (const char *path, unsigned char *blob, int blob_size);

    RL2_PRIVATE int
	rl2_decode_jpeg_scaled (int scale, const unsigned char *jpeg,
				int jpeg_sz, unsigned int *width,
				unsigned int *height,
				unsigned char *pixel_type,
				unsigned char **pixels, int *pixels_sz);

    RL2_PRIVATE int
	rl2_decode_webp_scaled (int scale, const unsigned char *webp,
				int webp_sz, unsigned int *width,
				unsigned int *height,
				unsigned char pixel_type,
				unsigned char **pixels, int *pixels_sz,
				unsigned char **mask, int *mask_sz);

    RL2_PRIVATE int
	rl2_decode_jpeg2000_scaled (int scale, const unsigned char *jpeg2000,
				    int jpeg2000_sz, unsigned int *width,
				    unsigned int *height,
				    unsigned char sample_type,
				    unsigned char pixel_type,
				    unsigned char num_bands,
				    unsigned char **pixels, int *pixels_sz);

    RL2_PRIVATE int
	rl2_data_to_png (const unsigned char *pixels, const unsigned char *mask,
			 double opacity, rl2PalettePtr plt,
			 unsigned int width, unsigned int height,
			 unsigned char sample_type, unsigned char pixel_type,
			 unsigned char num_bands, unsigned char **compr_data,
			 int *compressed_size);

    RL2_PRIVATE int
	rl2_decode_png (const unsigned char *png, int png_sz,
			unsigned int *width, unsigned int *height,
			unsigned char *sample_type, unsigned char *pixel_type,
			unsigned char *num_bands, unsigned char **pixels,
			int *pixels_sz, unsigned char **mask, int *mask_sz,
			rl2PalettePtr * palette);

    RL2_PRIVATE int
	rl2_data_to_charls (const unsigned char *pixels, unsigned int width,
			    unsigned int height, unsigned char sample_type,
			    unsigned char pixel_type, unsigned char num_bands,
			    unsigned char **compr_data, int *compressed_size);

    RL2_PRIVATE int
	rl2_decode_charls (const unsigned char *charls, int charls_sz,
			   unsigned int *width, unsigned int *height,
			   unsigned char *sample_type,
			   unsigned char *pixel_type, unsigned char *num_bands,
			   unsigned char **pixels, int *pixels_sz);

    RL2_PRIVATE int
	rl2_data_to_gif (const unsigned char *pixels,
			 rl2PalettePtr plt, unsigned int width,
			 unsigned int height, unsigned char sample_type,
			 unsigned char pixel_type, unsigned char **compr_data,
			 int *compressed_size);

    RL2_PRIVATE int
	rl2_decode_gif (const unsigned char *gif, int gif_sz,
			unsigned int *width, unsigned int *height,
			unsigned char *sample_type, unsigned char *pixel_type,
			unsigned char **pixels, int *pixels_sz,
			rl2PalettePtr * palette);

    RL2_PRIVATE int
	rl2_data_to_jpeg (const unsigned char *pixels,
			  const unsigned char *mask, rl2PalettePtr plt,
			  unsigned int width, unsigned int height,
			  unsigned char sample_type, unsigned char pixel_type,
			  unsigned char **jpeg, int *jpeg_size, int quality);

    RL2_PRIVATE int
	rl2_decode_tiff_mono4 (const unsigned char *tiff, int tiff_sz,
			       unsigned int *width, unsigned int *height,
			       unsigned char **pixels, int *pixels_sz);

    RL2_PRIVATE char truncate_8 (double val);

    RL2_PRIVATE unsigned char truncate_u8 (double val);

    RL2_PRIVATE short truncate_16 (double val);

    RL2_PRIVATE unsigned short truncate_u16 (double val);

    RL2_PRIVATE int truncate_32 (double val);

    RL2_PRIVATE unsigned int truncate_u32 (double val);

    RL2_PRIVATE void void_raw_buffer_palette (unsigned char *buffer,
					      unsigned int width,
					      unsigned int height,
					      rl2PixelPtr no_data);

    RL2_PRIVATE void void_raw_buffer (unsigned char *buffer,
				      unsigned int width,
				      unsigned int height,
				      unsigned char sample_type,
				      unsigned char num_bands,
				      rl2PixelPtr no_data);

    RL2_PRIVATE int rl2_load_dbms_tiles (sqlite3 * handle,
					 sqlite3_stmt * stmt_tiles,
					 sqlite3_stmt * stmt_data,
					 unsigned char *outbuf,
					 unsigned int width,
					 unsigned int height,
					 unsigned char sample_type,
					 unsigned char num_bands, double x_res,
					 double y_res, double minx, double miny,
					 double maxx, double maxy, int level,
					 int scale, rl2PalettePtr palette,
					 rl2PixelPtr no_data,
					 rl2RasterSymbolizerPtr style,
					 rl2RasterStatisticsPtr stats);

    RL2_PRIVATE int rl2_load_dbms_tiles_section (sqlite3 * handle,
						 sqlite3_int64 section_id,
						 sqlite3_stmt * stmt_tiles,
						 sqlite3_stmt * stmt_data,
						 unsigned char *outbuf,
						 unsigned int width,
						 unsigned int height,
						 unsigned char sample_type,
						 unsigned char num_bands,
						 double x_res, double y_res,
						 double minx, double maxy,
						 int scale,
						 rl2PalettePtr palette,
						 rl2PixelPtr no_data);

    RL2_PRIVATE void
	compute_aggregate_sq_diff (rl2RasterStatisticsPtr aggreg_stats);

    RL2_PRIVATE int get_coverage_sample_bands (sqlite3 * sqlite,
					       const char *coverage,
					       unsigned char *sample_type,
					       unsigned char *num_bands);

    RL2_PRIVATE int get_coverage_defs (sqlite3 * sqlite, const char *coverage,
				       unsigned int *tile_width,
				       unsigned int *tile_height,
				       unsigned char *sample_type,
				       unsigned char *pixel_type,
				       unsigned char *num_bands,
				       unsigned char *compression);

    RL2_PRIVATE rl2PixelPtr default_nodata (unsigned char sample,
					    unsigned char pixel,
					    unsigned char num_bands);

    RL2_PRIVATE WmsRetryListPtr alloc_retry_list ();

    RL2_PRIVATE void free_retry_list (WmsRetryListPtr lst);

    RL2_PRIVATE void add_retry (WmsRetryListPtr lst, double minx, double miny,
				double maxx, double maxy);

    RL2_PRIVATE int rl2_do_insert_levels (sqlite3 * handle, double base_res_x,
					  double base_res_y, double factor,
					  unsigned char sample_type,
					  sqlite3_stmt * stmt_levl);

    RL2_PRIVATE int rl2_do_insert_section_levels (sqlite3 * handle,
						  sqlite3_int64 section_id,
						  double base_res_x,
						  double base_res_y,
						  double factor,
						  unsigned char sample_type,
						  sqlite3_stmt * stmt_levl);

    RL2_PRIVATE int rl2_do_insert_stats (sqlite3 * handle,
					 rl2RasterStatisticsPtr section_stats,
					 sqlite3_int64 section_id,
					 sqlite3_stmt * stmt_upd_sect);

    RL2_PRIVATE int rl2_do_insert_section (sqlite3 * handle,
					   const char *src_path,
					   const char *section, int srid,
					   unsigned int width,
					   unsigned int height, double minx,
					   double miny, double maxx,
					   double maxy, char *xml_summary,
					   int section_paths, int section_md5,
					   int section_summary,
					   sqlite3_stmt * stmt_sect,
					   sqlite3_int64 * id);

    RL2_PRIVATE char *get_section_name (const char *src_path);

    RL2_PRIVATE rl2RasterPtr build_wms_tile (rl2CoveragePtr coverage,
					     const unsigned char *rgba_tile);

    RL2_PRIVATE int insert_wms_tile (InsertWmsPtr ptr, int *first,
				     rl2RasterStatisticsPtr * section_stats,
				     sqlite3_int64 * section_id);

    RL2_PRIVATE ResolutionsListPtr alloc_resolutions_list ();

    RL2_PRIVATE void destroy_resolutions_list (ResolutionsListPtr list);

    RL2_PRIVATE void add_base_resolution (ResolutionsListPtr list, int level,
					  int scale, double x_res,
					  double y_res);

    RL2_PRIVATE int rl2_find_best_resolution_level (sqlite3 * handle,
						    const char *coverage,
						    int by_section,
						    sqlite3_int64 section_id,
						    double x_res, double y_res,
						    int *level_id, int *scale,
						    int *real_scale,
						    double *xx_res,
						    double *yy_res);

    RL2_PRIVATE unsigned char get_palette_format (rl2PrivPalettePtr plt);

    RL2_PRIVATE int get_payload_from_monochrome_opaque (unsigned int width,
							unsigned int height,
							sqlite3 * handle,
							double minx,
							double miny,
							double maxx,
							double maxy, int srid,
							unsigned char *pixels,
							unsigned char format,
							int quality,
							unsigned char **image,
							int *image_sz);

    RL2_PRIVATE int get_payload_from_monochrome_transparent (unsigned int
							     width,
							     unsigned int
							     height,
							     unsigned char
							     *pixels,
							     unsigned char
							     format,
							     int quality,
							     unsigned char
							     **image,
							     int *image_sz,
							     double opacity);

    RL2_PRIVATE int get_payload_from_palette_opaque (unsigned int width,
						     unsigned int height,
						     sqlite3 * handle,
						     double minx, double miny,
						     double maxx, double maxy,
						     int srid,
						     unsigned char *pixels,
						     rl2PalettePtr palette,
						     unsigned char format,
						     int quality,
						     unsigned char **image,
						     int *image_sz);

    RL2_PRIVATE int get_payload_from_palette_transparent (unsigned int width,
							  unsigned int height,
							  unsigned char *pixels,
							  rl2PalettePtr palette,
							  unsigned char format,
							  int quality,
							  unsigned char **image,
							  int *image_sz,
							  unsigned char bg_red,
							  unsigned char
							  bg_green,
							  unsigned char
							  bg_blue,
							  double opacity);

    RL2_PRIVATE int get_payload_from_grayscale_opaque (unsigned int width,
						       unsigned int height,
						       sqlite3 * handle,
						       double minx, double miny,
						       double maxx, double maxy,
						       int srid,
						       unsigned char *pixels,
						       unsigned char format,
						       int quality,
						       unsigned char **image,
						       int *image_sz);

    RL2_PRIVATE int get_payload_from_grayscale_transparent (unsigned int
							    width,
							    unsigned int
							    height,
							    unsigned char
							    *pixels,
							    unsigned char
							    format, int quality,
							    unsigned char
							    **image,
							    int *image_sz,
							    unsigned char
							    bg_gray,
							    double opacity);

    RL2_PRIVATE int get_payload_from_rgb_opaque (unsigned int width,
						 unsigned int height,
						 sqlite3 * handle, double minx,
						 double miny, double maxx,
						 double maxy, int srid,
						 unsigned char *pixels,
						 unsigned char format,
						 int quality,
						 unsigned char **image,
						 int *image_sz);

    RL2_PRIVATE int get_payload_from_rgb_transparent (unsigned int width,
						      unsigned int height,
						      unsigned char *pixels,
						      unsigned char format,
						      int quality,
						      unsigned char **image,
						      int *image_sz,
						      unsigned char bg_red,
						      unsigned char bg_green,
						      unsigned char bg_blue,
						      double opacity);

    RL2_PRIVATE int get_rgba_from_monochrome_mask (unsigned int width,
						   unsigned int height,
						   unsigned char *pixels,
						   unsigned char *mask,
						   rl2PrivPixelPtr no_data,
						   unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_monochrome_opaque (unsigned int width,
						     unsigned int height,
						     unsigned char *pixels,
						     unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_monochrome_transparent (unsigned int width,
							  unsigned int height,
							  unsigned char *pixels,
							  unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_palette_mask (unsigned int base_width,
						unsigned int base_height,
						unsigned char *pixels,
						unsigned char *mask,
						rl2PalettePtr palette,
						rl2PrivPixelPtr no_data,
						unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_palette_opaque (unsigned int base_width,
						  unsigned int base_height,
						  unsigned char *pixels,
						  rl2PalettePtr palette,
						  unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_palette_transparent (unsigned int width,
						       unsigned int height,
						       unsigned char *pixels,
						       rl2PalettePtr palette,
						       unsigned char *rgba,
						       unsigned char bg_red,
						       unsigned char bg_green,
						       unsigned char bg_blue);

    RL2_PRIVATE int get_rgba_from_grayscale_mask (unsigned int width,
						  unsigned int height,
						  unsigned char *pixels,
						  unsigned char *mask,
						  rl2PrivPixelPtr no_data,
						  unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_grayscale_opaque (unsigned int width,
						    unsigned int height,
						    unsigned char *pixels,
						    unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_grayscale_transparent (unsigned int width,
							 unsigned int height,
							 unsigned char *pixels,
							 unsigned char *rgba,
							 unsigned char bg_gray);

    RL2_PRIVATE int get_rgba_from_rgb_mask (unsigned int width,
					    unsigned int height,
					    unsigned char *pixels,
					    unsigned char *mask,
					    rl2PrivPixelPtr no_data,
					    unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_rgb_opaque (unsigned int width,
					      unsigned int height,
					      unsigned char *pixels,
					      unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_rgb_transparent (unsigned int width,
						   unsigned int height,
						   unsigned char *pixels,
						   unsigned char *rgba,
						   unsigned char bg_red,
						   unsigned char bg_green,
						   unsigned char bg_blue);

    RL2_PRIVATE int get_rgba_from_datagrid_mask (unsigned int width,
						 unsigned int height,
						 unsigned char sample_type,
						 void *pixels,
						 unsigned char *mask,
						 rl2PrivPixelPtr no_made,
						 unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_multiband_mask (unsigned int width,
						  unsigned int height,
						  unsigned char sample_type,
						  unsigned char num_bands,
						  void *pixels,
						  unsigned char *mask,
						  rl2PrivPixelPtr no_made,
						  unsigned char *rgba);

    RL2_PRIVATE int get_payload_from_gray_rgba_opaque (unsigned int width,
						       unsigned int height,
						       sqlite3 * handle,
						       double minx, double miny,
						       double maxx, double maxy,
						       int srid,
						       unsigned char *rgb,
						       unsigned char format,
						       int quality,
						       unsigned char **image,
						       int *image_sz);

    RL2_PRIVATE int get_payload_from_gray_rgba_transparent (unsigned int
							    width,
							    unsigned int
							    height,
							    unsigned char *rgb,
							    unsigned char
							    *alpha,
							    unsigned char
							    format, int quality,
							    unsigned char
							    **image,
							    int *image_sz,
							    double opacity);

    RL2_PRIVATE int get_payload_from_rgb_rgba_opaque (unsigned int width,
						      unsigned int height,
						      sqlite3 * handle,
						      double minx, double miny,
						      double maxx, double maxy,
						      int srid,
						      unsigned char *rgb,
						      unsigned char format,
						      int quality,
						      unsigned char **image,
						      int *image_sz);

    RL2_PRIVATE int get_payload_from_rgb_rgba_transparent (unsigned int width,
							   unsigned int
							   height,
							   unsigned char *rgb,
							   unsigned char *alpha,
							   unsigned char format,
							   int quality,
							   unsigned char
							   **image,
							   int *image_sz,
							   double opacity);

    RL2_PRIVATE int build_rgb_alpha (unsigned int width,
				     unsigned int height, unsigned char *rgba,
				     unsigned char **rgb, unsigned char **alpha,
				     unsigned char bg_red,
				     unsigned char bg_green,
				     unsigned char bg_blue);

    RL2_PRIVATE int get_rgba_from_multiband8 (unsigned int width,
					      unsigned int height,
					      unsigned char red_band,
					      unsigned char green_band,
					      unsigned char blue_band,
					      unsigned char num_bands,
					      unsigned char *pixels,
					      unsigned char *mask,
					      rl2PrivPixelPtr no_data,
					      unsigned char *rgba);

    RL2_PRIVATE int get_rgba_from_multiband16 (unsigned int width,
					       unsigned int height,
					       unsigned char red_band,
					       unsigned char green_band,
					       unsigned char blue_band,
					       unsigned char num_bands,
					       unsigned short *pixels,
					       unsigned char *mask,
					       rl2PrivPixelPtr no_data,
					       unsigned char *rgba);
    RL2_PRIVATE int parse_worldfile (FILE * in, double *px, double *py,
				     double *pres_x, double *pres_y);

    RL2_PRIVATE rl2CoverageStylePtr coverage_style_from_xml (char *name,
							     unsigned char
							     *xml);

    RL2_PRIVATE rl2FeatureTypeStylePtr feature_type_style_from_xml (char *name,
								    unsigned
								    char *xml);

    RL2_PRIVATE rl2GroupStylePtr group_style_from_sld_xml (char *name,
							   unsigned char *xml);

    RL2_PRIVATE rl2PrivCoverageStylePtr
	rl2_create_default_coverage_style (void);

    RL2_PRIVATE rl2PrivRasterSymbolizerPtr
	rl2_create_default_raster_symbolizer (void);

    RL2_PRIVATE rl2PrivVectorSymbolizerPtr
	rl2_create_default_vector_symbolizer (void);

    RL2_PRIVATE rl2PrivStrokePtr rl2_create_default_stroke (void);

    RL2_PRIVATE rl2PrivFillPtr rl2_create_default_fill (void);

    RL2_PRIVATE rl2PrivColorReplacementPtr
	rl2_create_default_color_replacement (void);

    RL2_PRIVATE rl2PrivGraphicItemPtr
	rl2_create_default_external_graphic (void);

    RL2_PRIVATE rl2PrivGraphicItemPtr rl2_create_default_mark (void);

    RL2_PRIVATE rl2PrivGraphicPtr rl2_create_default_graphic (void);

    RL2_PRIVATE rl2PrivPointPlacementPtr
	rl2_create_default_point_placement (void);

    RL2_PRIVATE rl2PrivLinePlacementPtr
	rl2_create_default_line_placement (void);

    RL2_PRIVATE rl2PrivHaloPtr rl2_create_default_halo (void);

    RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
	rl2_create_default_point_symbolizer (void);

    RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
	rl2_create_default_line_symbolizer (void);

    RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
	rl2_create_default_polygon_symbolizer (void);

    RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
	rl2_create_default_text_symbolizer (void);

    RL2_PRIVATE void rl2_destroy_raster_symbolizer (rl2PrivRasterSymbolizerPtr
						    symbolizer);

    RL2_PRIVATE void rl2_destroy_vector_symbolizer (rl2PrivVectorSymbolizerPtr
						    symbolizer);

    RL2_PRIVATE void
	rl2_destroy_vector_symbolizer_item (rl2PrivVectorSymbolizerItemPtr
					    item);

    RL2_PRIVATE void rl2_destroy_stroke (rl2PrivStrokePtr stroke);

    RL2_PRIVATE void rl2_destroy_fill (rl2PrivFillPtr fill);

    RL2_PRIVATE void rl2_destroy_color_replacement (rl2PrivColorReplacementPtr
						    repl);

    RL2_PRIVATE void rl2_destroy_external_graphic (rl2PrivExternalGraphicPtr
						   ext);

    RL2_PRIVATE void rl2_destroy_mark (rl2PrivMarkPtr mark);

    RL2_PRIVATE void rl2_destroy_graphic_item (rl2PrivGraphicItemPtr item);

    RL2_PRIVATE void rl2_destroy_graphic (rl2PrivGraphicPtr graphic);

    RL2_PRIVATE void rl2_destroy_point_placement (rl2PrivPointPlacementPtr
						  place);

    RL2_PRIVATE void rl2_destroy_line_placement (rl2PrivLinePlacementPtr place);

    RL2_PRIVATE void rl2_destroy_halo (rl2PrivHaloPtr halo);

    RL2_PRIVATE void rl2_destroy_point_symbolizer (rl2PrivPointSymbolizerPtr
						   symbolizer);

    RL2_PRIVATE void rl2_destroy_line_symbolizer (rl2PrivLineSymbolizerPtr
						  symbolizer);

    RL2_PRIVATE void rl2_destroy_polygon_symbolizer (rl2PrivPolygonSymbolizerPtr
						     symbolizer);

    RL2_PRIVATE void rl2_destroy_text_symbolizer (rl2PrivTextSymbolizerPtr
						  symbolizer);

    RL2_PRIVATE rl2PrivRuleSingleArgPtr
	rl2_create_default_rule_single_arg (void);

    RL2_PRIVATE rl2PrivRuleLikeArgsPtr rl2_create_default_rule_like_args (void);

    RL2_PRIVATE rl2PrivRuleBetweenArgsPtr
	rl2_create_default_rule_between_args (void);

    RL2_PRIVATE rl2PrivStyleRulePtr rl2_create_default_style_rule (void);

    RL2_PRIVATE void rl2_destroy_style_rule (rl2PrivStyleRulePtr rule);

    RL2_PRIVATE void rl2_destroy_rule_like_args (rl2PrivRuleLikeArgsPtr like);

    RL2_PRIVATE void rl2_destroy_rule_between_args (rl2PrivRuleBetweenArgsPtr
						    between);

    RL2_PRIVATE void rl2_destroy_rule_single_arg (rl2PrivRuleSingleArgPtr
						  single);

    RL2_PRIVATE int get_raster_band_histogram (rl2PrivBandStatisticsPtr band,
					       unsigned char **image,
					       int *image_sz);

    RL2_PRIVATE int rl2_copy_raw_pixels (rl2RasterPtr raster,
					 unsigned char *outbuf,
					 unsigned int width,
					 unsigned int height,
					 unsigned char sample_type,
					 unsigned char num_bands, double x_res,
					 double y_res, double minx, double maxy,
					 double tile_minx, double tile_maxy,
					 rl2PixelPtr no_data,
					 rl2RasterSymbolizerPtr style,
					 rl2RasterStatisticsPtr stats);

    RL2_PRIVATE unsigned char *rl2_copy_endian_raw_pixels (const unsigned char
							   *pixels,
							   int pixels_sz,
							   unsigned int width,
							   unsigned int height,
							   unsigned char
							   sample_type,
							   unsigned char
							   num_bands,
							   int big_endian);

    RL2_PRIVATE int rl2_build_shaded_relief_mask (sqlite3 * handle,
						  rl2CoveragePtr cvg,
						  double relief_factor,
						  double scale_factor,
						  unsigned int width,
						  unsigned int height,
						  double minx, double miny,
						  double maxx, double maxy,
						  double x_res, double y_res,
						  float **shaded_relief,
						  int *shaded_relief_sz);

    RL2_PRIVATE int set_coverage_infos (sqlite3 * handle,
					const char *coverage_name,
					const char *title,
					const char *abstract);

    RL2_PRIVATE int rl2_test_layer_group (sqlite3 * handle,
					  const char *group_name);

    RL2_PRIVATE int rl2_rgba_raster_data (sqlite3 * handle,
					  const char *coverage_name, void *ctx,
					  int level, double minx, double miny,
					  double maxx, double maxy,
					  rl2PalettePtr palette,
					  rl2PixelPtr no_data);

    RL2_PRIVATE int rl2_aux_render_image (struct aux_renderer *aux,
					  unsigned char **ximage,
					  int *ximage_size);

    RL2_PRIVATE void rl2_aux_group_renderer (struct aux_group_renderer *auxgrp);

    RL2_PRIVATE double rl2_get_shaded_relief_scale_factor (sqlite3 * handle,
							   const char
							   *coverage);

    RL2_PRIVATE void *rl2_CreateMD5Checksum (void);

    RL2_PRIVATE void rl2_FreeMD5Checksum (void *p_md5);

    RL2_PRIVATE void rl2_UpdateMD5Checksum (void *p_md5,
					    const unsigned char *blob,
					    int blob_len);

    RL2_PRIVATE char *rl2_FinalizeMD5Checksum (void *p_md5);

    RL2_PRIVATE int rl2_is_mixed_resolutions_coverage (sqlite3 * handle,
						       const char *coverage);

    RL2_PRIVATE int rl2_has_styled_rgb_colors (rl2RasterSymbolizerPtr style);

    RL2_PRIVATE int rl2_get_raw_raster_data_common (sqlite3 * handle,
						    rl2CoveragePtr cvg,
						    int by_section,
						    sqlite3_int64 section_id,
						    unsigned int width,
						    unsigned int height,
						    double minx, double miny,
						    double maxx, double maxy,
						    double x_res, double y_res,
						    unsigned char **buffer,
						    int *buf_size,
						    rl2PalettePtr * palette,
						    unsigned char out_pixel,
						    rl2PixelPtr bgcolor,
						    rl2RasterSymbolizerPtr
						    style,
						    rl2RasterStatisticsPtr
						    stats);

    RL2_PRIVATE char *rl2_double_quoted_sql (const char *value);

    RL2_PRIVATE int rl2_parse_point (sqlite3 * sqlite,
				     const unsigned char *blob, int blob_sz,
				     double *x, double *y);

    RL2_PRIVATE int rl2_parse_point_generic (sqlite3 * sqlite,
					     const unsigned char *blob,
					     int blob_sz, double *x, double *y);

    RL2_PRIVATE int rl2_parse_bbox_srid (sqlite3 * sqlite,
					 const unsigned char *blob, int blob_sz,
					 int *srid, double *minx, double *miny,
					 double *maxx, double *maxy);

    RL2_PRIVATE int rl2_parse_bbox (sqlite3 * sqlite, const unsigned char *blob,
				    int blob_sz, double *minx, double *miny,
				    double *maxx, double *maxy);

    RL2_PRIVATE int rl2_build_bbox (sqlite3 * sqlite, int srid, double minx,
				    double miny, double maxx, double maxy,
				    unsigned char **blob, int *blob_sz);

    RL2_PRIVATE int rl2_delta_encode (unsigned char *buffer, int size,
				      int distance);

    RL2_PRIVATE int rl2_delta_decode (unsigned char *buffer, int size,
				      int distance);

#ifdef __cplusplus
}
#endif

#endif				/* _RASTERLITE2_PRIVATE_H */
