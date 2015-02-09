/*

 rl2auxgeom -- auxiliary methods supporting basic 2D geometries

 version 0.1, 2015 February 2

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
 
Portions created by the Initial Developer are Copyright (C) 2015
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
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#include <spatialite/gg_const.h>

/*
/
/ PLEASE NOTE: all these functions are roughly equivalent to the 
/ corresponding APIs implemented in libspatialite.
/ this odd code duplication is absolutely required in order to carefully
/ avoid to create any direct reference to link symbols declared by
/ libspatialite, because such an action will irremediabely introduce
/ nasty dependencies to libsqlite3 as well.
/ and this is absolutely not allowed in a "pure" loadable module context.
/
/ a very important difference exists between the RasterLite2 and the
/ SpatiaLite implementations.
/ in RL2 any XYZ, XYM or XYZM dimension will always be parsed as if it
/ was just XY; this is fully consintent with RL2 specific requirements
/ (just rendering 2D geoms via Cairo)
/
*/

static int
rl2GeomEndianArch ()
{
/* checking if target CPU is a little-endian one */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 0;
    return 1;
}

static rl2PointPtr
rl2CreatePoint (double x, double y)
{
/* POINT object constructor */
    rl2PointPtr p = malloc (sizeof (rl2Point));
    p->x = x;
    p->y = y;
    p->next = NULL;
    return p;
}

static void
rl2DestroyPoint (rl2PointPtr ptr)
{
/* POINT object destructor */
    if (ptr != NULL)
	free (ptr);
}

static rl2LinestringPtr
rl2CreateLinestring (int vert)
{
/* LINESTRING object constructor */
    rl2LinestringPtr p = malloc (sizeof (rl2Linestring));
    p->coords = malloc (sizeof (double) * (vert * 2));
    p->points = vert;
    p->next = NULL;
    return p;
}

static void
rl2DestroyLinestring (rl2LinestringPtr ptr)
{
/* LINESTRING object desctructror */
    if (ptr)
      {
	  if (ptr->coords)
	      free (ptr->coords);
	  free (ptr);
      }
}

static rl2RingPtr
rl2CreateRing (int vert)
{
/* ring object constructor */
    rl2RingPtr p = malloc (sizeof (rl2Ring));
    p->coords = malloc (sizeof (double) * (vert * 2));
    p->points = vert;
    p->next = NULL;
    return p;
}

static void
rl2DestroyRing (rl2RingPtr ptr)
{
/* ring object destructor */
    if (ptr)
      {
	  if (ptr->coords)
	      free (ptr->coords);
	  free (ptr);
      }
}

static rl2PolygonPtr
rl2CreatePolygon (int vert, int excl)
{
/* POLYGON object constructor */
    rl2PolygonPtr p;
    rl2RingPtr pP;
    int ind;
    p = malloc (sizeof (rl2Polygon));
    p->exterior = rl2CreateRing (vert);
    p->num_interiors = excl;
    p->next = NULL;
    if (excl == 0)
	p->interiors = NULL;
    else
	p->interiors = malloc (sizeof (rl2Ring) * excl);
    for (ind = 0; ind < p->num_interiors; ind++)
      {
	  pP = p->interiors + ind;
	  pP->points = 0;
	  pP->coords = NULL;
      }
    return p;
}

static void
rl2DestroyPolygon (rl2PolygonPtr p)
{
/* POLYGON object destructor */
    rl2RingPtr pP;
    int ind;
    if (p->exterior)
	rl2DestroyRing (p->exterior);
    for (ind = 0; ind < p->num_interiors; ind++)
      {
	  pP = p->interiors + ind;
	  if (pP->coords)
	      free (pP->coords);
      }
    if (p->interiors)
	free (p->interiors);
    free (p);
}

static rl2GeometryPtr
rl2CreateGeometry ()
{
/* GEOMETRYCOLLECTION object constructor */
    rl2GeometryPtr p = malloc (sizeof (rl2Geometry));
    p->first_point = NULL;
    p->last_point = NULL;
    p->first_linestring = NULL;
    p->last_linestring = NULL;
    p->first_polygon = NULL;
    p->last_polygon = NULL;
    return p;
}

static int
rl2GeomImport32 (const unsigned char *p, int little_endian,
		 int little_endian_arch)
{
/* fetches a 32bit int from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
      }
    return convert.int_value;
}

static float
rl2GeomImportF32 (const unsigned char *p, int little_endian,
		  int little_endian_arch)
{
/* fetches a 32bit float from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	float flt_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
      }
    return convert.flt_value;
}

static double
rl2GeomImport64 (const unsigned char *p, int little_endian,
		 int little_endian_arch)
{
/* fetches a 64bit double from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[8];
	double double_value;
    } convert;
    if (little_endian_arch)
      {
/* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 7);
		convert.byte[1] = *(p + 6);
		convert.byte[2] = *(p + 5);
		convert.byte[3] = *(p + 4);
		convert.byte[4] = *(p + 3);
		convert.byte[5] = *(p + 2);
		convert.byte[6] = *(p + 1);
		convert.byte[7] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
		convert.byte[4] = *(p + 4);
		convert.byte[5] = *(p + 5);
		convert.byte[6] = *(p + 6);
		convert.byte[7] = *(p + 7);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
		convert.byte[4] = *(p + 4);
		convert.byte[5] = *(p + 5);
		convert.byte[6] = *(p + 6);
		convert.byte[7] = *(p + 7);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 7);
		convert.byte[1] = *(p + 6);
		convert.byte[2] = *(p + 5);
		convert.byte[3] = *(p + 4);
		convert.byte[4] = *(p + 3);
		convert.byte[5] = *(p + 2);
		convert.byte[6] = *(p + 1);
		convert.byte[7] = *(p + 0);
	    }
      }
    return convert.double_value;
}

static void
rl2GeomExport32 (unsigned char *p, int value, int little_endian,
		 int little_endian_arch)
{
/* stores a 32bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
      }
}

static void
rl2GeomExport64 (unsigned char *p, double value, int little_endian,
		 int little_endian_arch)
{
/* stores a 64bit double into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[8];
	double double_value;
    } convert;
    convert.double_value = value;
    if (little_endian_arch)
      {
/* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 7) = convert.byte[0];
		*(p + 6) = convert.byte[1];
		*(p + 5) = convert.byte[2];
		*(p + 4) = convert.byte[3];
		*(p + 3) = convert.byte[4];
		*(p + 2) = convert.byte[5];
		*(p + 1) = convert.byte[6];
		*(p + 0) = convert.byte[7];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
		*(p + 4) = convert.byte[4];
		*(p + 5) = convert.byte[5];
		*(p + 6) = convert.byte[6];
		*(p + 7) = convert.byte[7];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
		*(p + 4) = convert.byte[4];
		*(p + 5) = convert.byte[5];
		*(p + 6) = convert.byte[6];
		*(p + 7) = convert.byte[7];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 7) = convert.byte[0];
		*(p + 6) = convert.byte[1];
		*(p + 5) = convert.byte[2];
		*(p + 4) = convert.byte[3];
		*(p + 3) = convert.byte[4];
		*(p + 2) = convert.byte[5];
		*(p + 1) = convert.byte[6];
		*(p + 0) = convert.byte[7];
	    }
      }
}

static void
rl2AddPointToGeometry (rl2GeometryPtr p, double x, double y)
{
/* adding a LINESTRING to this GEOMETRYCOLLECTION */
    rl2PointPtr point = rl2CreatePoint (x, y);
    if (p->first_point == NULL)
	p->first_point = point;
    if (p->last_point != NULL)
	p->last_point->next = point;
    p->last_point = point;
}

static void
rl2ParsePoint (rl2GeometryPtr geom, const unsigned char *blob, int size,
	       int endian, int endian_arch, int *offset)
{
/* decodes a POINT from WKB */
    double x;
    double y;
    if (size < *offset + 16)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    *offset += 16;
    rl2AddPointToGeometry (geom, x, y);
}

static void
rl2ParsePointZ (rl2GeometryPtr geom, const unsigned char *blob, int size,
		int endian, int endian_arch, int *offset)
{
/* decodes a POINTZ from WKB */
    double x;
    double y;
    if (size < *offset + 24)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    *offset += 24;
    rl2AddPointToGeometry (geom, x, y);
}

static void
rl2ParsePointM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		int endian, int endian_arch, int *offset)
{
/* decodes a POINTM from WKB */
    double x;
    double y;
    if (size < *offset + 24)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    *offset += 24;
    rl2AddPointToGeometry (geom, x, y);
}

static void
rl2ParsePointZM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		 int endian, int endian_arch, int *offset)
{
/* decodes a POINTZM from WKB */
    double x;
    double y;
    if (size < *offset + 32)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    *offset += 32;
    rl2AddPointToGeometry (geom, x, y);
}

static rl2LinestringPtr
rl2AddLinestringToGeometry (rl2GeometryPtr p, int vert)
{
/* adding a POINT to this GEOMETRYCOLLECTION */
    rl2LinestringPtr line = rl2CreateLinestring (vert);
    if (p->first_linestring == NULL)
	p->first_linestring = line;
    if (p->last_linestring != NULL)
	p->last_linestring->next = line;
    p->last_linestring = line;
    return line;
}

static void
rl2ParseLine (rl2GeometryPtr geom, const unsigned char *blob, int size,
	      int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRING from WKB */
    int points;
    int iv;
    double x;
    double y;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (16 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  rl2SetPoint (line->coords, iv, x, y);
	  *offset += 16;
      }
}

static void
rl2ParseLineZ (rl2GeometryPtr geom, const unsigned char *blob, int size,
	       int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRINGZ from WKB */
    int points;
    int iv;
    double x;
    double y;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (24 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  rl2SetPoint (line->coords, iv, x, y);
	  *offset += 24;
      }
}

static void
rl2ParseLineM (rl2GeometryPtr geom, const unsigned char *blob, int size,
	       int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRINGM from WKB */
    int points;
    int iv;
    double x;
    double y;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (24 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  rl2SetPoint (line->coords, iv, x, y);
	  *offset += 24;
      }
}

static void
rl2ParseLineZM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRINGZM from WKB */
    int points;
    int iv;
    double x;
    double y;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (32 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  rl2SetPoint (line->coords, iv, x, y);
	  *offset += 32;
      }
}

static rl2PolygonPtr
rl2AddPolygonToGeometry (rl2GeometryPtr p, int vert, int interiors)
{
/* adding a POLYGON to this GEOMETRYCOLLECTION */
    rl2PolygonPtr polyg = rl2CreatePolygon (vert, interiors);
    if (p->first_polygon == NULL)
	p->first_polygon = polyg;
    if (p->last_polygon != NULL)
	p->last_polygon->next = polyg;
    p->last_polygon = polyg;
    return polyg;
}

static rl2RingPtr
rl2AddInteriorRing (rl2PolygonPtr p, int pos, int vert)
{
/* adding an interior ring to some polygon */
    rl2RingPtr pP = p->interiors + pos;
    pP->points = vert;
    pP->coords = malloc (sizeof (double) * (vert * 2));
    return pP;
}

static void
rl2ParsePolygon (rl2GeometryPtr geom, const unsigned char *blob, int size,
		 int endian, int endian_arch, int *offset)
{
/* decodes a POLYGON from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (16 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 16;
		rl2SetPoint (ring->coords, iv, x, y);
	    }
      }
}

static void
rl2ParsePolygonZ (rl2GeometryPtr geom, const unsigned char *blob, int size,
		  int endian, int endian_arch, int *offset)
{
/* decodes a POLYGONZ from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (24 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 24;
		rl2SetPoint (ring->coords, iv, x, y);
	    }
      }
}

static void
rl2ParsePolygonM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		  int endian, int endian_arch, int *offset)
{
/* decodes a POLYGONM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (24 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 24;
		rl2SetPoint (ring->coords, iv, x, y);
	    }
      }
}

static void
rl2ParsePolygonZM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		   int endian, int endian_arch, int *offset)
{
/* decodes a POLYGONZM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (32 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 32;
		rl2SetPoint (ring->coords, iv, x, y);
	    }
      }
}

static void
rl2ParseCompressedLine (rl2GeometryPtr geom, const unsigned char *blob,
			int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRING from WKB */
    int points;
    int iv;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (8 * points) + 16)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 16;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		*offset += 8;
	    }
	  rl2SetPoint (line->coords, iv, x, y);
	  last_x = x;
	  last_y = y;
      }
}

static void
rl2ParseCompressedLineZ (rl2GeometryPtr geom, const unsigned char *blob,
			 int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRINGZ from WKB */
    int points;
    int iv;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (12 * points) + 24)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 24;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		*offset += 12;
	    }
	  rl2SetPoint (line->coords, iv, x, y);
	  last_x = x;
	  last_y = y;
      }
}

static void
rl2ParseCompressedLineM (rl2GeometryPtr geom, const unsigned char *blob,
			 int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRINGM from WKB */
    int points;
    int iv;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (16 * points) + 16)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 24;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		*offset += 16;
	    }
	  rl2SetPoint (line->coords, iv, x, y);
	  last_x = x;
	  last_y = y;
      }
}

static void
rl2ParseCompressedLineZM (rl2GeometryPtr geom, const unsigned char *blob,
			  int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRINGZM from WKB */
    int points;
    int iv;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (20 * points) + 24)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 32;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		*offset += 20;
	    }
	  rl2SetPoint (line->coords, iv, x, y);
	  last_x = x;
	  last_y = y;
      }
}

static void
rl2ParseCompressedPolygon (rl2GeometryPtr geom, const unsigned char *blob,
			   int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGON from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (8 * nverts) + 16)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      *offset += 16;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      *offset += 8;
		  }
		rl2SetPoint (ring->coords, iv, x, y);
		last_x = x;
		last_y = y;
	    }
      }
}

static void
rl2ParseCompressedPolygonZ (rl2GeometryPtr geom, const unsigned char *blob,
			    int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGONZ from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (12 * nverts) + 24)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      *offset += 24;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      *offset += 12;
		  }
		rl2SetPoint (ring->coords, iv, x, y);
		last_x = x;
		last_y = y;
	    }
      }
}

static void
rl2ParseCompressedPolygonM (rl2GeometryPtr geom, const unsigned char *blob,
			    int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGONM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (16 * nverts) + 16)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      *offset += 24;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      *offset += 16;
		  }
		rl2SetPoint (ring->coords, iv, x, y);
		last_x = x;
		last_y = y;
	    }
      }
}

static void
rl2ParseCompressedPolygonZM (rl2GeometryPtr geom, const unsigned char *blob,
			     int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGONZM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (20 * nverts) + 24)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      *offset += 32;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      *offset += 20;
		  }
		rl2SetPoint (ring->coords, iv, x, y);
		last_x = x;
		last_y = y;
	    }
      }
}

static void
rl2ParseGeometry (rl2GeometryPtr geom, const unsigned char *blob, int size,
		  int endian, int endian_arch, int *offset)
{
/* decodes a MULTIxx or GEOMETRYCOLLECTION from SpatiaLite BLOB */
    int entities;
    int type;
    int ie;
    if (size < *offset + 4)
	return;
    entities = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ie = 0; ie < entities; ie++)
      {
	  if (size < *offset + 5)
	      return;
	  type = rl2GeomImport32 (blob + *offset + 1, endian, endian_arch);
	  *offset += 5;
	  switch (type)
	    {
	    case GAIA_POINT:
		rl2ParsePoint (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POINTZ:
		rl2ParsePointZ (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POINTM:
		rl2ParsePointM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POINTZM:
		rl2ParsePointZM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRING:
		rl2ParseLine (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRINGZ:
		rl2ParseLineZ (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRINGM:
		rl2ParseLineM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRINGZM:
		rl2ParseLineZM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POLYGON:
		rl2ParsePolygon (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POLYGONZ:
		rl2ParsePolygonZ (geom, blob, size, endian, endian_arch,
				  offset);
		break;
	    case GAIA_POLYGONM:
		rl2ParsePolygonM (geom, blob, size, endian, endian_arch,
				  offset);
		break;
	    case GAIA_POLYGONZM:
		rl2ParsePolygonZM (geom, blob, size, endian, endian_arch,
				   offset);
		break;
	    case GAIA_COMPRESSED_LINESTRING:
		rl2ParseCompressedLine (geom, blob, size, endian, endian_arch,
					offset);
		break;
	    case GAIA_COMPRESSED_LINESTRINGZ:
		rl2ParseCompressedLineZ (geom, blob, size, endian, endian_arch,
					 offset);
		break;
	    case GAIA_COMPRESSED_LINESTRINGM:
		rl2ParseCompressedLineM (geom, blob, size, endian, endian_arch,
					 offset);
		break;
	    case GAIA_COMPRESSED_LINESTRINGZM:
		rl2ParseCompressedLineZM (geom, blob, size, endian, endian_arch,
					  offset);
		break;
	    case GAIA_COMPRESSED_POLYGON:
		rl2ParseCompressedPolygon (geom, blob, size, endian,
					   endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_POLYGONZ:
		rl2ParseCompressedPolygonZ (geom, blob, size, endian,
					    endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_POLYGONM:
		rl2ParseCompressedPolygonM (geom, blob, size, endian,
					    endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_POLYGONZM:
		rl2ParseCompressedPolygonZM (geom, blob, size, endian,
					     endian_arch, offset);
		break;
	    default:
		break;
	    };
      }
}

RL2_PRIVATE rl2GeometryPtr
rl2_geometry_from_blob (const unsigned char *blob, int size)
{
/* decoding from SpatiaLite BLOB to GEOMETRY */
    int type;
    int little_endian;
    int offset;
    int endian_arch = rl2GeomEndianArch ();
    rl2GeometryPtr geom = NULL;
    if (size < 45)
	return NULL;		/* cannot be an internal BLOB WKB geometry */
    if (*(blob + 0) != GAIA_MARK_START)
	return NULL;		/* failed to recognize START signature */
    if (*(blob + (size - 1)) != GAIA_MARK_END)
	return NULL;		/* failed to recognize END signature */
    if (*(blob + 38) != GAIA_MARK_MBR)
	return NULL;		/* failed to recognize MBR signature */
    if (*(blob + 1) == GAIA_LITTLE_ENDIAN)
	little_endian = 1;
    else if (*(blob + 1) == GAIA_BIG_ENDIAN)
	little_endian = 0;
    else
	return NULL;		/* unknown encoding; nor little-endian neither big-endian */
    type = rl2GeomImport32 (blob + 39, little_endian, endian_arch);
    geom = rl2CreateGeometry ();
    offset = 43;
    switch (type)
      {
	  /* parsing elementary geometries */
      case GAIA_POINT:
	  rl2ParsePoint (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_POINTZ:
	  rl2ParsePointZ (geom, blob, size, little_endian, endian_arch,
			  &offset);
	  break;
      case GAIA_POINTM:
	  rl2ParsePointM (geom, blob, size, little_endian, endian_arch,
			  &offset);
	  break;
      case GAIA_POINTZM:
	  rl2ParsePointZM (geom, blob, size, little_endian, endian_arch,
			   &offset);
	  break;
      case GAIA_LINESTRING:
	  rl2ParseLine (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_LINESTRINGZ:
	  rl2ParseLineZ (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_LINESTRINGM:
	  rl2ParseLineM (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_LINESTRINGZM:
	  rl2ParseLineZM (geom, blob, size, little_endian, endian_arch,
			  &offset);
	  break;
      case GAIA_POLYGON:
	  rl2ParsePolygon (geom, blob, size, little_endian, endian_arch,
			   &offset);
	  break;
      case GAIA_POLYGONZ:
	  rl2ParsePolygonZ (geom, blob, size, little_endian, endian_arch,
			    &offset);
	  break;
      case GAIA_POLYGONM:
	  rl2ParsePolygonM (geom, blob, size, little_endian, endian_arch,
			    &offset);
	  break;
      case GAIA_POLYGONZM:
	  rl2ParsePolygonZM (geom, blob, size, little_endian, endian_arch,
			     &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRING:
	  rl2ParseCompressedLine (geom, blob, size, little_endian, endian_arch,
				  &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRINGZ:
	  rl2ParseCompressedLineZ (geom, blob, size, little_endian, endian_arch,
				   &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRINGM:
	  rl2ParseCompressedLineM (geom, blob, size, little_endian, endian_arch,
				   &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRINGZM:
	  rl2ParseCompressedLineZM (geom, blob, size, little_endian,
				    endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGON:
	  rl2ParseCompressedPolygon (geom, blob, size, little_endian,
				     endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGONZ:
	  rl2ParseCompressedPolygonZ (geom, blob, size, little_endian,
				      endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGONM:
	  rl2ParseCompressedPolygonM (geom, blob, size, little_endian,
				      endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGONZM:
	  rl2ParseCompressedPolygonZM (geom, blob, size, little_endian,
				       endian_arch, &offset);
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  rl2ParseGeometry (geom, blob, size, little_endian, endian_arch,
			    &offset);
	  break;
      default:
	  break;
      };
    return geom;
}

RL2_PRIVATE void
rl2_destroy_geometry (rl2GeometryPtr geom)
{
/* GEOMETRYCOLLECTION object destructor */
    rl2PointPtr pP;
    rl2PointPtr pPn;
    rl2LinestringPtr pL;
    rl2LinestringPtr pLn;
    rl2PolygonPtr pA;
    rl2PolygonPtr pAn;
    if (geom == NULL)
	return;
    pP = geom->first_point;
    while (pP != NULL)
      {
	  pPn = pP->next;
	  rl2DestroyPoint (pP);
	  pP = pPn;
      }
    pL = geom->first_linestring;
    while (pL != NULL)
      {
	  pLn = pL->next;
	  rl2DestroyLinestring (pL);
	  pL = pLn;
      }
    pA = geom->first_polygon;
    while (pA != NULL)
      {
	  pAn = pA->next;
	  rl2DestroyPolygon (pA);
	  pA = pAn;
      }
    free (geom);
}

RL2_PRIVATE int
rl2_serialize_linestring (rl2LinestringPtr line, unsigned char **result,
			  int *size)
{
/* serializing a BLOB Geometry - linestring */
    int iv;
    unsigned char *ptr;
    int endian_arch = rl2GeomEndianArch ();
    double minx = DBL_MAX;
    double maxx = 0.0 - DBL_MAX;
    double miny = DBL_MAX;
    double maxy = 0.0 - DBL_MAX;
    double x;
    double y;

    *result = NULL;
    *size = 0;
    if (line == NULL)
	return 0;

/* computing the MBR */
    for (iv = 0; iv < line->points; iv++)
      {
	  rl2GetPoint (line->coords, iv, &x, &y);
	  if (x < minx)
	      minx = x;
	  if (x > maxx)
	      maxx = x;
	  if (y < miny)
	      miny = y;
	  if (y > maxy)
	      maxy = y;
      }
/* computing the size of BLOB */
    *size = 44;			/* header size */
    *size += (4 + ((sizeof (double) * 2) * line->points));	/* # points + [x,y] for each vertex */
    *result = malloc (*size);
    ptr = *result;
/* building the BLOB */
    *ptr = GAIA_MARK_START;	/* START signature */
    *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
    rl2GeomExport32 (ptr + 2, 4326, 1, endian_arch);	/* the SRID */
    rl2GeomExport64 (ptr + 6, minx, 1, endian_arch);	/* MBR - minimum X */
    rl2GeomExport64 (ptr + 14, miny, 1, endian_arch);	/* MBR - minimum Y */
    rl2GeomExport64 (ptr + 22, maxx, 1, endian_arch);	/* MBR - maximum X */
    rl2GeomExport64 (ptr + 30, maxy, 1, endian_arch);	/* MBR - maximum Y */
    *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
    rl2GeomExport32 (ptr + 39, GAIA_LINESTRING, 1, endian_arch);	/* class LINESTRING */
    rl2GeomExport32 (ptr + 43, line->points, 1, endian_arch);	/* # points */
    ptr += 47;
    for (iv = 0; iv < line->points; iv++)
      {
	  rl2GetPoint (line->coords, iv, &x, &y);
	  rl2GeomExport64 (ptr, x, 1, endian_arch);
	  rl2GeomExport64 (ptr + 8, y, 1, endian_arch);
	  ptr += 16;
      }
    *ptr = GAIA_MARK_END;	/* END signature */
    return 1;
}

RL2_PRIVATE int
rl2_serialize_ring (rl2RingPtr ring, unsigned char **result, int *size)
{
/* serializing a BLOB Geometry - polygon ring */
    int iv;
    unsigned char *ptr;
    int endian_arch = rl2GeomEndianArch ();
    double minx = DBL_MAX;
    double maxx = 0.0 - DBL_MAX;
    double miny = DBL_MAX;
    double maxy = 0.0 - DBL_MAX;
    double x;
    double y;

    *result = NULL;
    *size = 0;
    if (ring == NULL)
	return 0;

/* computing the MBR */
    for (iv = 0; iv < ring->points; iv++)
      {
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  if (x < minx)
	      minx = x;
	  if (x > maxx)
	      maxx = x;
	  if (y < miny)
	      miny = y;
	  if (y > maxy)
	      maxy = y;
      }
/* computing the size of BLOB */
    *size = 44;			/* header size */
    *size += (8 + ((sizeof (double) * 2) * ring->points));	/* # rings + # points + [x.y] array - exterior ring */
    *result = malloc (*size);
    ptr = *result;
/* building the BLOB */
    *ptr = GAIA_MARK_START;	/* START signature */
    *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
    rl2GeomExport32 (ptr + 2, -1, 1, endian_arch);	/* the SRID */
    rl2GeomExport64 (ptr + 6, minx, 1, endian_arch);	/* MBR - minimum X */
    rl2GeomExport64 (ptr + 14, miny, 1, endian_arch);	/* MBR - minimum Y */
    rl2GeomExport64 (ptr + 22, maxx, 1, endian_arch);	/* MBR - maximum X */
    rl2GeomExport64 (ptr + 30, maxy, 1, endian_arch);	/* MBR - maximum Y */
    *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
    rl2GeomExport32 (ptr + 39, GAIA_POLYGON, 1, endian_arch);	/* class POLYGON */
    rl2GeomExport32 (ptr + 43, 1, 1, endian_arch);	/* # rings */
    rl2GeomExport32 (ptr + 47, ring->points, 1, endian_arch);	/* # points - exterior ring */
    ptr += 51;
    for (iv = 0; iv < ring->points; iv++)
      {
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - exterior ring */
	  rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - exterior ring */
	  ptr += 16;
      }
    *ptr = GAIA_MARK_END;	/* END signature */
    return 1;
}
