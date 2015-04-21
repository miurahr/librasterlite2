/* 
 rl2graphics.h -- RasterLite2 common graphics support
  
 version 2.0, 2013 September 29

 Author: Sandro Furieri a.furieri@lqt.it

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

The Original Code is the RasterLite2 library

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

/**
 \file rl2graphics.h

 Graphics (Cairo) support
 */

#ifndef _RL2GRAPHICS_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2GRAPHICS_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define RL2_FONTSTYLE_NORMAL	5101
#define RL2_FONTSTYLE_ITALIC	5102
#define RL2_FONTSTYLE_OBLIQUE	5103

#define RL2_FONTWEIGHT_NORMAL	5201
#define RL2_FONTWEIGHT_BOLD	5202

#define RL2_CLEAR_PATH		5100
#define RL2_PRESERVE_PATH	5101

#define RL2_PEN_CAP_BUTT	5210
#define RL2_PEN_CAP_ROUND	5211
#define RL2_PEN_CAP_SQUARE	5212

#define RL2_PEN_JOIN_MITER	5261
#define RL2_PEN_JOIN_ROUND	5262
#define RL2_PEN_JOIN_BEVEL	5263

    typedef struct rl2_graphics_context rl2GraphicsContext;
    typedef rl2GraphicsContext *rl2GraphicsContextPtr;

    typedef struct rl2_graphics_pattern rl2GraphicsPattern;
    typedef rl2GraphicsPattern *rl2GraphicsPatternPtr;

    typedef struct rl2_graphics_font rl2GraphicsFont;
    typedef rl2GraphicsFont *rl2GraphicsFontPtr;

    typedef struct rl2_graphics_bitmap rl2GraphicsBitmap;
    typedef rl2GraphicsBitmap *rl2GraphicsBitmapPtr;

/**
 Creates a generic Graphics Context 

 \param width canvass width (in pixels)
 \param height canvass height (in pixels)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_destroy_context, rl2_graph_create_svg_context,
 rl2_graph_create_pdf_context, rl2_graph_create_mem_pdf_context
 
 \note you are responsible to destroy (before or after) any Graphics Context
 returned by rl2_graph_create_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr rl2_graph_create_context (int width,
								int height);

/**
 Destroys a Graphics Context object freeing any allocated resource 

 \param handle the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context()
 
 \sa rl2_graph_create_context
 */
    RL2_DECLARE void rl2_graph_destroy_context (rl2GraphicsContextPtr context);

/**
 Creates an SVG Graphics Context 

 \param path pathname of the target SVG output file
 \param width canvass width (in pixels)
 \param height canvass height (in pixels)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_create_context, rl2_graph_destroy_context, rl2_graph_create_pdf_context,
 rl2_graph_create_mem_pdf_context
 
 \note you are responsible to destroy (before or after) any SVG Graphics Context
 returned by rl2_graph_create_svg_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr rl2_graph_create_svg_context (const char
								    *path,
								    int width,
								    int height);

/**
 Creates a PDF Graphics Context 

 \param path pathname of the target PDF output file
 \param dpi the PDF printing resolution measured in DPI 
 \param page_width total page width (in inches) including any margin
 \param page_height total page height (in inches) including any margin
 \param margin_width horizontal margin width (in inches)
 \param margin_height vertical margin height (in inches)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, rl2_graph_create_mem_pdf_context,
 rl2_graph_destroy_context
 
 \note you are responsible to destroy (before or after) any PDF Graphics Context
 returned by rl2_graph_create_pdf_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr rl2_graph_create_pdf_context (const char
								    *path,
								    int dpi,
								    double
								    page_width,
								    double
								    page_height,
								    double
								    margin_width,
								    double
								    margin_height);

/**
 Creates an in-memory PDF Graphics Context 

 \param mem handle to an in-memory PDF target create by rl2_create_mem_pdf_target()
 \param dpi the PDF printing resolution measured in DPI 
 \param page_width total page width (in inches) including any margin
 \param page_height total page height (in inches) including any margin
 \param margin_width horizontal margin width (in inches)
 \param margin_height vertical margin height (in inches)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, rl2_graph_create_pdf_context,
 rl2_graph_destroy_context, rl2_create_mem_pdf_target
 
 \note you are responsible to destroy (before or after) any PDF Graphics Context
 returned by rl2_graph_create_mem_pdf_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr
	rl2_graph_create_mem_pdf_context (rl2MemPdfPtr pdf, int dpi,
					  double page_width, double page_height,
					  double margin_width,
					  double margin_height);

/**
 Create an in-memory PDF target
 
 \return the handle to an initially empty in-memory PDF target: NULL on failure
 
 \sa rl2_graph_create_mem_pdf_context, rl2_destroy_mem_pdf_target,
 rl2_get_mem_pdf_buffer
 
 \note you are responsible to destroy (before or after) any in-memory PDF target
 returned by rl2_create_mem_pdf_target() by invoking rl2_destroy_mem_pdf_target().
 */
    RL2_DECLARE rl2MemPdfPtr rl2_create_mem_pdf_target (void);

/**
 Destroys an in-memory PDF target freeing any allocated resource 

 \param handle the pointer to a valid in-memory PDF target returned by a previous call
 to rl2_create_mem_pdf_target()
 
 \sa rl2_create_mem_pdf_target, rl2_destroy_mem_pdf_target
 */
    RL2_DECLARE void rl2_destroy_mem_pdf_target (rl2MemPdfPtr target);

/**
 Transfers the ownership of the memory buffer contained into a in-memory PDF target

 \param handle the pointer to a valid in-memory PDF target returned by a previous call
 to rl2_create_mem_pdf_target()
 \param buffer on completion this variable will point to the a memory block 
  containing the PDF document (may be NULL if any error occurred).
 \param size on completion this variable will contain the size (in bytes)
 of the memory block containing the PDF document.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing an empty in-memory PDF target).
 
 \sa rl2_create_mem_pdf_target
 
 \note after invoking rl2_get_mem_pdf_buffer() the in-memory PDF target
 will be reset to its initial empty state-
 */
    RL2_DECLARE int rl2_get_mem_pdf_buffer (rl2MemPdfPtr target,
					    unsigned char **buffer, int *size);

/**
 Selects the currently set Pen for a Graphics Context (solid style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param red Pen color: red component.
 \param green Pen color: green component.
 \param blue Pen color: blue component.
 \param alpha Pen transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_set_solid_pen (rl2GraphicsContextPtr context,
					     unsigned char red,
					     unsigned char green,
					     unsigned char blue,
					     unsigned char alpha, double width,
					     int line_cap, int line_join);

/**
 Selects the currently set Pen for a Graphics Context (dashed style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param red Pen color: red component.
 \param green Pen color: green component.
 \param blue Pen color: blue component.
 \param alpha Pen transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL
 \param dash_count the total count of dash_list items
 \param dash_list an array of dash items 
 \param dash_offset start offset

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_set_dashed_pen (rl2GraphicsContextPtr context,
					      unsigned char red,
					      unsigned char green,
					      unsigned char blue,
					      unsigned char alpha, double width,
					      int line_cap, int line_join,
					      int dash_count,
					      double dash_list[],
					      double dash_offset);

/**
 Selects the currently set Linear Gradient Pen for a Graphics Context (solid style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param x start point (X coord, in pixels) of the Gradient.
 \param y start point (Y coord, in pixels) of the Gradient.
 \param width the Gradient width.
 \param height the Gradient height.
 \param red1 first Gradient color: red component.
 \param green1 first Gradient color: green component.
 \param blue1 first Gradient color: blue component.
 \param alpha1 first Gradient color transparency (0 full transparent; 255 full opaque).
 \param red2 second Gradient color: red component.
 \param green2 second Gradient color: green component.
 \param blue2 second Gradient color: blue component.
 \param alpha2 second Gradient color transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_brush, 
 rl2_graph_set_dashed_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font

 \note a Pen created by this function always is a Linear Gradient Pen.
 */
    RL2_DECLARE int
	rl2_graph_set_linear_gradient_solid_pen (rl2GraphicsContextPtr,
						 double x, double y,
						 double width, double height,
						 unsigned char red1,
						 unsigned char green1,
						 unsigned char blue1,
						 unsigned char alpha1,
						 unsigned char red2,
						 unsigned char green2,
						 unsigned char blue2,
						 unsigned char alpha2,
						 double pen_width, int line_cap,
						 int line_join);

/**
 Selects the currently set Linear Gradient Pen for a Graphics Context (dashed style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param x start point (X coord, in pixels) of the Gradient.
 \param y start point (Y coord, in pixels) of the Gradient.
 \param width the Gradient width.
 \param height the Gradient height.
 \param red1 first Gradient color: red component.
 \param green1 first Gradient color: green component.
 \param blue1 first Gradient color: blue component.
 \param alpha1 first Gradient color transparency (0 full transparent; 255 full opaque).
 \param red2 second Gradient color: red component.
 \param green2 second Gradient color: green component.
 \param blue2 second Gradient color: blue component.
 \param alpha2 second Gradient color transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL
 \param dash_count the total count of dash_list items
 \param dash_list an array of dash items 
 \param dash_offset start offset

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, 
 rl2_graph_set_dashed_pen, rl2_graph_set_linear_gradient_solid_pen, 
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font

 \note a Pen created by this function always is a Linear Gradient Pen.
 */
    RL2_DECLARE int
	rl2_graph_set_linear_gradient_dashed_pen (rl2GraphicsContextPtr,
						  double x, double y,
						  double width, double height,
						  unsigned char red1,
						  unsigned char green1,
						  unsigned char blue1,
						  unsigned char alpha1,
						  unsigned char red2,
						  unsigned char green2,
						  unsigned char blue2,
						  unsigned char alpha2,
						  double pen_width,
						  int line_cap, int line_join,
						  int dash_count,
						  double dash_list[],
						  double offset);

/**
 Selects the currently set Pattern Pen for a Graphics Context (solid style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param pattern the pointer to a valid Graphics Pattern returned by a previous
 call to rl2_graph_create_pattern().
 \param pen_width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_dashed_pen, rl2_graph_set_brush, rl2_graph_set_font, 
 rl2_create_pattern, rl2_graph_realease_pattern_pen

 \note a Pen created by this function always is a Pattern Pen, 
 i.e. a Pen repeatedly using a small bitmap as a filling source.
 */
    RL2_DECLARE int rl2_graph_set_pattern_solid_pen (rl2GraphicsContextPtr
						     context,
						     rl2GraphicsPatternPtr
						     pattern, double width,
						     int line_cap,
						     int line_join);

/**
 Selects the currently set Pattern Pen for a Graphics Context (dashed style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param pattern the pointer to a valid Graphics Pattern returned by a previous
 call to rl2_graph_create_pattern().
 \param pen_width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL
 \param dash_count the total count of dash_list items
 \param dash_list an array of dash items 
 \param dash_offset start offset

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_brush, rl2_graph_set_font, 
 rl2_create_pattern, rl2_graph_realease_pattern_pen

 \note a Pen created by this function always is a Pattern Pen, 
 i.e. a Pen repeatedly using a small bitmap as a filling source.
 */
    RL2_DECLARE int rl2_graph_set_pattern_dashed_pen (rl2GraphicsContextPtr
						      context,
						      rl2GraphicsPatternPtr
						      pattern, double width,
						      int line_cap,
						      int line_join,
						      int dash_count,
						      double dash_list[],
						      double offset);

/**
 Releases the currently set Pattern Pen (if any) from a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen

 \note you should always release a Pattern Pen before attempting to
 destroy the corresponding Pattern object.
 */
    RL2_DECLARE int rl2_graph_release_pattern_pen (rl2GraphicsContextPtr
						   context);

/**
 Selects the currently set Brush for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param red Brush color: red component.
 \param green Brush color: green component.
 \param blue Brush color: blue component.
 \param alpha Brush transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rrl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_pen, rl2_graph_set_pattern_pen,
 rl2_graph_set_linear_gradient_brush, rl2_graph_set_pattern_brush,
 rl2_graph_set_font

 \note a Brush created by this function always is a solid Brush, may be
 supporting transparency.
 */
    RL2_DECLARE int rl2_graph_set_brush (rl2GraphicsContextPtr context,
					 unsigned char red, unsigned char green,
					 unsigned char blue,
					 unsigned char alpha);

/**
 Selects the currently set Brush for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param x start point (X coord, in pixels) of the Gradient.
 \param y start point (Y coord, in pixels) of the Gradient.
 \param width the Gradient width.
 \param height the Gradient height.
 \param red1 first Gradient color: red component.
 \param green1 first Gradient color: green component.
 \param blue1 first Gradient color: blue component.
 \param alpha1 first Gradient color transparency (0 full transparent; 255 full opaque).
 \param red2 second Gradient color: red component.
 \param green2 second Gradient color: green component.
 \param blue2 second Gradient color: blue component.
 \param alpha2 second Gradient color transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_pen, rl2_graph_set_pattern_pen,
 rl2_graph_set_brush, rl2_graph_set_pattern_brush,
 rl2_graph_set_font

 \note a Brush created by this function always is a Linear Gradient Brush.
 */
    RL2_DECLARE int rl2_graph_set_linear_gradient_brush (rl2GraphicsContextPtr,
							 double x, double y,
							 double width,
							 double height,
							 unsigned char red1,
							 unsigned char green1,
							 unsigned char blue1,
							 unsigned char alpha1,
							 unsigned char red2,
							 unsigned char green2,
							 unsigned char blue2,
							 unsigned char alpha2);

/**
 Selects the currently set Brush for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param pattern the pointer to a valid Graphics Pattern returned by a previous
 call to rl2_graph_create_pattern().

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_brush, rl2_graph_set_linear_gradient_brush, 
 rl2_graph_set_font, rl2_create_pattern, rl2_graph_release_pattern_brush

 \note a Brush created by this function always is a Pattern Brush, 
 i.e. a Brush repeatedly using a small bitmap as a filling source.
 */
    RL2_DECLARE int rl2_graph_set_pattern_brush (rl2GraphicsContextPtr context,
						 rl2GraphicsPatternPtr pattern);

/**
 Releases the currently set Pattern Brush (if any) from a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_pattern_brush

 \note you should always release a Pattern Brush before attempting to
 destroy the corresponding Pattern object.
 */
    RL2_DECLARE int rl2_graph_release_pattern_brush (rl2GraphicsContextPtr
						     context);

/**
 Selects the currently set Font for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param font the pointer to a valid Graphics Font returned by a previous call
 to rl2_create_font().

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_brush, rl2_graph_set_linear_gradient_brush, rl2_graph_set_pattern_brush,
 rl2_create_font
 */
    RL2_DECLARE int rl2_graph_set_font (rl2GraphicsContextPtr context,
					rl2GraphicsFontPtr font);

/**
 Releases the currently set Font out from a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_font, rl2_graph_destroy_font
 
 \note you must always call rl2_graph_release_font() before attempting
 to destroy a currently selected TrueType Font.
 */
    RL2_DECLARE int rl2_graph_release_font (rl2GraphicsContextPtr context);

/**
 Creates a Pattern Brush 

 \param rgbaArray pointer to an array of RGBA pixels representing the bitmap
 supporting the Pattern Brush to be created.
 \param width Pattern width (in pixels)
 \param height Pattern height (in pixels)

 \return the pointer to the corresponding Pattern Brush object: NULL on failure
 
 \sa rl2_graph_set_pattern_brush, rl2_graph_destroy_brush,
 rl2_create_pattern_from_external_graphic
 
 \note you are responsible to destroy (before or after) any Pattern Brush
 returned by rl2_graph_create_pattern() by invoking rl2_graph_destroy_pattern().
 */
    RL2_DECLARE rl2GraphicsPatternPtr rl2_graph_create_pattern (unsigned char
								*rgbaArray,
								int width,
								int height);

/**
 Creates a Pattern Brush from an External Graphic resource (PNG, GIF, JPEG)

 \param handle SQLite3 connection handle
 \param xlink_href unique External Graphic identifier

 \return the pointer to the corresponding Pattern Brush object: NULL on failure
 
 \sa rl2_graph_set_pattern_brush, rl2_graph_destroy_brush,
 rl2_graph_create_pattern
 
 \note you are responsible to destroy (before or after) any Pattern Brush
 returned by rl2_create_pattern_from_external_graphic() by invoking 
 rl2_graph_destroy_pattern().
 */
    RL2_DECLARE rl2GraphicsPatternPtr
	rl2_create_pattern_from_external_graphic (sqlite3 * handle,
						  const char *xlink_href);

/**
 Destroys a Pattern Brush object freeing any allocated resource 

 \param handle the pointer to a valid Pattern Brush returned by a previous call
 to rl2_graph_create_pattern()
 
 \sa rl2_graph_create_pattern

 \note you should always release a Pattern Pen or Pattern Brush from any
 referencing Graphic Context before attempting to destroy the corresponding
 Pattern object.
 */
    RL2_DECLARE void rl2_graph_destroy_pattern (rl2GraphicsPatternPtr pattern);

/**
 Recolors a Monochrome Pattern object  

 \param handle the pointer to a valid Pattern returned by a previous call
 to rl2_graph_create_pattern() or rl2_create_pattern_from_external_graphic()
 \param r the new Red component value to be set
 \param g the new Green component value
 \param b the new Blue component value
 
 \return RL2_OK on success: RL2_ERROR on failure. 
 
 \sa rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic,
 rl2_graph_destroy_pattern

 \note only a Monochrome Pattern can be succesfully recolored.
 Completely transparent pixels will be ignored; all opaque pixels
 should declare exactly the same color.
 */
    RL2_DECLARE int rl2_graph_pattern_recolor (rl2GraphicsPatternPtr pattern,
					       unsigned char r, unsigned char g,
					       unsigned char b);

/**
 Sets half-transparency for a Pattern object  

 \param handle the pointer to a valid Pattern returned by a previous call
 to rl2_graph_create_pattern() or rl2_create_pattern_from_external_graphic()
 \param alpha transparency (0 full transparent; 255 full opaque).
 
 \return RL2_OK on success: RL2_ERROR on failure. 
 
 \sa rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic,
 rl2_graph_destroy_pattern

 \note Completely transparent pixels will be always preserved as such; 
 all other pixels will assume the required transparency.
 */
    RL2_DECLARE int rl2_graph_pattern_transparency (rl2GraphicsPatternPtr
						    pattern,
						    unsigned char alpha);

/**
 Creates a Graphics Font Object (CAIRO built-in "toy" fonts)

 \param facename the font Facename. Expected to be one of "serif", 
 "sans-serif" or "monospace". a NULL facename will default to "monospace".
 \param size the Graphics Font size (in points).
 \param style one of RL2_FONTSTYLE_NORMAL, RL2_FONTSTYLE_ITALIC or
 RL2_FONTSTYLE_OBLIQUE
 \param weight one of RL2_FONTWEIGHT_NORMAL or RL2_FONTWEIGHT_BOLD

 \return the pointer to the corresponding Font object: NULL on failure
 
 \sa rl2_graph_set_font, 
 rl2_graph_destroy_font, rl2_graph_font_set_color,
 rl2_graph_font_set_color, rl2_graph_font_set_halo
 
 \note you are responsible to destroy (before or after) any Pattern Brush
 returned by rl2_graph_create_toy_font() by invoking rl2_graph_destroy_font().
 */
    RL2_DECLARE rl2GraphicsFontPtr rl2_graph_create_toy_font (const char
							      *facename,
							      double size,
							      int style,
							      int weight);

/**
 Will retrieve a BLOB-encoded TrueType Font by its FaceName
 
 \param handle SQLite3 connection handle
 \param fontname name of the required font: e.g. "Roboto-BoldItalic"
 \param font on completion this variable will point to the a memory block 
  containing the BLOB-encoded TrueType font (may be NULL if any error occurred).
 \param font_sz on completion this variable will contain the size (in bytes)
 of the memory block containing the BLOB-encoded TrueType font.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including requesting a not existing TrueType font).
 
 \note you are responsible to free (before or after) the BLOB-encode
 font returned by rl2_get_TrueType_font()
 */

    RL2_DECLARE int rl2_get_TrueType_font (sqlite3 * handle,
					   const char *facename,
					   unsigned char **font, int *font_sz);

/**
 Creates a Graphics Font Object (TrueType fonts)

 \param priv_data pointer to the opaque internal connection object 
 returned by a previous call to rl2_alloc_private() 
 \param ttf the TrueType font definition (internal BLOB format).
 \param ttf_bytes length (in bytes) of the above TTF definition
 \param size the Graphics Font size (in points).

 \return the pointer to the corresponding Font object: NULL on failure
 
 \sa rl2_graph_create_toy_font, rl2_get_TrueType_font, 
 rl2_search_TrueType_font, rl2_graph_set_font, rl2_graph_destroy_font, 
 rl2_graph_font_set_color, rl2_graph_font_set_color, 
 rl2_graph_font_set_halo
 
 \note you are responsible to destroy (before or after) any TrueType
 font returned by rl2_graph_create_TrueType_font() by invoking 
 rl2_graph_destroy_font().
 */
    RL2_DECLARE rl2GraphicsFontPtr rl2_graph_create_TrueType_font (const void
								   *priv_data,
								   const
								   unsigned char
								   *ttf,
								   int
								   ttf_bytes,
								   double size);

/**
 Searches and creates a Graphics Font Object (TrueType fonts)

 \param handle SQLite3 connection handle
 \param priv_data pointer to the opaque internal connection object 
 returned by a previous call to rl2_alloc_private() 
 \param fontname name of the required font: e.g. "Roboto-BoldItalic"
 \param size the Graphics Font size (in points).

 \return the pointer to the corresponding Font object: NULL on failure
 
 \sa rl2_graph_create_toy_font, rl2_get_TrueType_font, 
 rl2_graph_create_TrueType_font, rl2_graph_set_font, 
 rl2_graph_destroy_font, rl2_graph_font_set_color,
 rl2_graph_font_set_color, rl2_graph_font_set_halo
 
 \note you are responsible to destroy (before or after) any TrueType
 font returned by rl2_search_TrueType_font() by invoking 
 rl2_graph_destroy_font().
 */
    RL2_DECLARE rl2GraphicsFontPtr rl2_search_TrueType_font (sqlite3 * handle,
							     const void
							     *priv_data,
							     const char
							     *facename,
							     double size);

/**
 Destroys a Graphics Font object freeing any allocated resource 

 \param handle the pointer to a valid Font returned by a previous call
 to rl2_graph_create_toy_font() or rl2_graph_create_TrueType_font()
 
 \sa rl2_graph_create_toy_font, rl2_graph_create_TrueType_font
 
 \note you must always call rl2_graph_release_font() before attempting
 to destroy the currently selected font if it is of the TrueType class.
 */
    RL2_DECLARE void rl2_graph_destroy_font (rl2GraphicsFontPtr font);

/**
 Selects the currently set Color for a Graphics Font

 \param font the pointer to a valid Graphics Font returned by a previous call
 to rl2_graph_create_toy_font() or rl2_graph_create_TrueType_font()
 \param red Font color: red component.
 \param green Font color: green component.
 \param blue Font color: blue component.
 \param alpha Font transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_toy_font, rl2_graph_create_TrueType_font,
 rl2_graph_font_set_halo, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_font_set_color (rl2GraphicsFontPtr font,
					      unsigned char red,
					      unsigned char green,
					      unsigned char blue,
					      unsigned char alpha);

/**
 Selects the currently set Halo for a Graphics Font

 \param font the pointer to a valid Graphics Font returned by a previous call
 to rl2_graph_create_toy_font() or rl2_graph_create_TrueType_font()
 \param radius the Halo Radius (in points); declaring a zero or negative value
 will remove the Halo from the Font.
 \param red Halo color: red component.
 \param green Halo color: green component.
 \param blue Halo color: blue component.
 \param alpha Halo transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_toy_font, rl2_graph_create_TrueType_font, 
 rl2_graph_font_set_color, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_font_set_halo (rl2GraphicsFontPtr font,
					     double radius,
					     unsigned char red,
					     unsigned char green,
					     unsigned char blue,
					     unsigned char alpha);

/**
 Creates a Graphics Bitmap 

 \param rgbaArray pointer to an array of RGBA pixels representing the bitmap.
 \param width Bitmap width (in pixels)
 \param height Bitmap height (in pixels)

 \return the pointer to the corresponding Graphics Bitmap object: NULL on failure
 
 \sa rl2_graph_destroy_bitmap
 
 \note you are responsible to destroy (before or after) any Graphics Bitmap
 returned by rl2_graph_create_bitmap() by invoking rl2_graph_destroy_bitmap().
 */
    RL2_DECLARE rl2GraphicsBitmapPtr rl2_graph_create_bitmap (unsigned char
							      *rgbaArray,
							      int width,
							      int height);

/**
 Destroys a Graphics Bitmap object freeing any allocated resource 

 \param handle the pointer to a valid Font returned by a previous call
 to rl2_graph_create_bitmap()
 
 \sa rl2_graph_create_bitmap
 */
    RL2_DECLARE void rl2_graph_destroy_bitmap (rl2GraphicsBitmapPtr bitmap);

/**
 Drawing a Rectangle into a canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param x the X coordinate of the top left corner of the rectangle.
 \param y the Y coordinate of the top left corner of the rectangle.
 \param width the width of the rectangle.
 \param height the height of the rectangle.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush,
 rl2_graph, draw_rectangle, rl2_graph_draw_ellipse, rl2_graph_draw_circle_sector, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the rectangle will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_rectangle (rl2GraphicsContextPtr context,
					      double x, double y, double width,
					      double height);

/**
 Drawing a Rectangle with rounded corners into a canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param x the X coordinate of the top left corner of the rectangle.
 \param y the Y coordinate of the top left corner of the rectangle.
 \param width the width of the rectangle.
 \param height the height of the rectangle.
 \param radius used for rounded corners.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush,
 rl2_draw_rectangle, rl2_graph_draw_ellipse, rl2_graph_draw_circle_sector, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the rectangle will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_rounded_rectangle (rl2GraphicsContextPtr
						      context, double x,
						      double y, double width,
						      double height,
						      double radius);

/**
 Drawing an Ellipse into a canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param x the X coordinate of the top left corner of the rectangle
 into which the ellipse is inscribed.
 \param y the Y coordinate of the top left corner of the rectangle.
 \param width the width of the rectangle.
 \param height the height of the rectangle.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush, 
 rl2_draw_rectangle, rl2_graph_draw_rounded_rectangle, rl2_graph_draw_circle_sector, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the ellipse will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_ellipse (rl2GraphicsContextPtr context,
					    double x, double y, double width,
					    double height);

/**
 Drawing a Circular Sector into a canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param center_x the X coordinate of the circle's centre.
 \param center_y the Y coordinate of the circle's centre.
 \param radius the circle's radius.
 \param from_angle the angle (in degrees) at wich the sector starts.
 \param to_angle the angle (in degrees) at wich the sector ends.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush, 
 rl2_draw_rectangle, rl2_graph_draw_rounded_rectangle, rl2_graph_draw_ellipse, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the Sector will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_circle_sector (rl2GraphicsContextPtr context,
						  double center_x,
						  double center_y,
						  double radius,
						  double from_angle,
						  double to_angle);

/**
 Drawing a straight Line into a canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param x0 the X coordinate of the first point.
 \param y0 the Y coordinate of the first point.
 \param x1 the X coordinate of the second point.
 \param y1 the Y coordinate of the second point.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush, 
 rl2_draw_rectangle, rl2_graph_draw_rounded_rectangle, rl2_graph_draw_ellipse, 
 rl2_graph_draw_circular_sector, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the Sector will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_stroke_line (rl2GraphicsContextPtr context,
					   double x0, double y0, double x1,
					   double y1);

/**
 Begins a new SubPath

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param x the X coordinate of the point.
 \param y the Y coordinate of the point.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_add_line_to_path,
 rl2_graph_close_subpath, rl2_graph_fill_path, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_move_to_point (rl2GraphicsContextPtr context,
					     double x, double y);

/**
 Add a line into the current SubPath

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param x the X coordinate of the destination point.
 \param y the Y coordinate of the destination point.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_brush, rl2_graph_move_to_point,
 rl2_graph_close_subpath, rl2_graph_fill_path, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_add_line_to_path (rl2GraphicsContextPtr context,
						double x, double y);

/**
 Terminates the current SubPath

 \param context the pointer to a valid Graphics Context (aka Canvass).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_brush, rl2_graph_move_to_point,
 rl2_graph_add_line_to_path, rl2_graph_fill_path, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_close_subpath (rl2GraphicsContextPtr context);

/**
 Fills the current Path using the currently set Brush

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param preserve if true the current Path will be preserved into the
 current Canvass, otherwise it will be definitely removed.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_brush, rl2_graph_move_to_point,
 rl2_graph_add_line_to_path, rl2_graph_close_subpath, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_fill_path (rl2GraphicsContextPtr context,
					 int preserve);

/**
 Strokes the current Path using the currently set Pen

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param preserve if true the current Path will be preserved into the
 current Canvass, otherwise it will be definitely removed.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_move_to_point,
 rl2_graph_add_line_to_path, rl2_graph_close_subpath, rl2_graph_fill_path
 */
    RL2_DECLARE int rl2_graph_stroke_path (rl2GraphicsContextPtr context,
					   int preserve);

/**
 Draws a text into the Canvass using the currently set Font

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param text string to be printed into the canvass.
 \param x the X coordinate of the top left corner of the text.
 \param y the Y coordinate of the top left corner of the text.
 \param angle an angle (in degrees) to rotate the text

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_font, rl2_graph_get_text_extent
 */
    RL2_DECLARE int rl2_graph_draw_text (rl2GraphicsContextPtr context,
					 const char *text, double x, double y,
					 double angle);

/**
 Computes the extent corresponding to a text into the Canvass using the currently set Font

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param text string to be printed into the canvass.
 \param pre_x on completion this variable will contain the horizontal 
  spacing before the text.
 \param pre_y on completion this variable will contain the vertical
  spacing before the text.
 \param width on completion this variable will contain the width of 
 the printed text.
 \param width on completion this variable will contain the height of 
 the printed text.
 \param post_x on completion this variable will contain the horizontal
  spacing after the text.
 \param post_y on completion this variable will contain the vertical
  spacing after the text.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_font, rl2_graph_get_text_extent
 */
    RL2_DECLARE int rl2_graph_get_text_extent (rl2GraphicsContextPtr context,
					       const char *text, double *pre_x,
					       double *pre_y, double *width,
					       double *height, double *post_x,
					       double *post_y);

/**
 Draws a Bitmap into the Canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param bitmap the pointer to a valid Graphics Bitmap to be rendered.
 \param x the X coordinate of the top left corner of the image.
 \param y the Y coordinate of the top left corner of the image.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_bitmap, rl2_graph_destroy_bitmap,
 rl2_graph_draw_rescaled_bitmap
 */
    RL2_DECLARE int rl2_graph_draw_bitmap (rl2GraphicsContextPtr context,
					   rl2GraphicsBitmapPtr bitmap, int x,
					   int y);

/**
 Draws a Rescaled Bitmap into the Canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).
 \param bitmap the pointer to a valid Graphics Bitmap to be rendered.
 \param scale_x the Scale Factor for X axis
 \param scale_y the Scale Factor for Y axis
 \param x the X coordinate of the top left corner of the image.
 \param y the Y coordinate of the top left corner of the image.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_bitmap, rl2_graph_destroy_bitmap,
 rl2_graph_draw_bitmap
 */
    RL2_DECLARE int rl2_graph_draw_rescaled_bitmap (rl2GraphicsContextPtr
						    context,
						    rl2GraphicsBitmapPtr bitmap,
						    double scale_x,
						    double scale_y, int x,
						    int y);

/**
 Creates an RGB Array corresponding to the current Canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).

 \return the pointer to the RGB Array: NULL on failure.
 
 \sa rl2_graph_get_context_alpha_array
 
 \note you are responsible to destroy (before or after) any RGB Array
 returned by rl2_graph_get_context_rgb_array() by invoking free().
 */
    RL2_DECLARE unsigned char
	*rl2_graph_get_context_rgb_array (rl2GraphicsContextPtr context);

/**
 Creates an Array of Alpha values corresponding to the current Canvass

 \param context the pointer to a valid Graphics Context (aka Canvass).

 \return the pointer to the Array of Alpha Values: NULL on failure.
 
 \sa rl2_graph_get_context_rgb_array
 
 \note you are responsible to destroy (before or after) any RGB Array
 returned by rl2_graph_get_context_alpha_array() by invoking free().
 */
    RL2_DECLARE unsigned char
	*rl2_graph_get_context_alpha_array (rl2GraphicsContextPtr context);

#ifdef __cplusplus
}
#endif

#endif				/* _RL2GPAPHICS_H */
