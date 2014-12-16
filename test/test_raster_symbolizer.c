/*

 test_raster_symbolizer.c -- RasterLite-2 Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"

static int
test_group_renderer (sqlite3 * sqlite, rl2GroupStylePtr style, int ind,
		     int *retcode)
{
/* testing GroupRendered */
    rl2GroupRendererPtr renderer = rl2_create_group_renderer (sqlite, style);
    if (renderer != NULL)
      {
	  rl2_destroy_group_renderer (renderer);
	  fprintf (stderr, "Unexpected Valid GroupRenderer #%d\n", ind);
	  *retcode += 39;
	  return 0;
      }
    return 1;
}

static int
test_group_style (sqlite3 * db_handle, int no_web_connection, int *retcode)
{
/* loading and testing Group Styles */
    char *sql;
    int ret;
    int xret = 0;
    sqlite3_stmt *stmt1;
    sqlite3_stmt *stmt2;
    sqlite3_int64 last_id;
    const char *group = "my_group";
    const char *path1 = "group_style_1.xml";
    const char *path2 = "group_style_2.xml";
    const char *str;
    rl2GroupStylePtr style;
    int valid;
    int count;

/* loading the Group Stles */
    if (no_web_connection)
	sql = "SELECT SE_RegisterGroupStyle(XB_Create(XB_LoadXML(?), 1))";
    else
	sql = "SELECT SE_RegisterGroupStyle(XB_Create(XB_LoadXML(?), 1, 1))";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt1, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Unable to create the SQL statement (GroupStyle)\n");
	  *retcode += 1;
	  return 0;
      }

    sql = "SELECT SE_RegisterStyledGroupStyle(?, ?)";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt2, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "Unable to create the SQL statement (StyledGroupStyle)\n");
	  *retcode += 2;
	  return 0;
      }

    sqlite3_reset (stmt1);
    sqlite3_clear_bindings (stmt1);
    sqlite3_bind_text (stmt1, 1, path1, strlen (path1), SQLITE_STATIC);
    ret = sqlite3_step (stmt1);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt1, 0) == 1)
	      xret = 1;
      }
    if (xret != 1)
      {
	  fprintf (stderr, "Unable to load \"%s\"\n", path2);
	  *retcode += 3;
	  return 0;
      }
    last_id = sqlite3_last_insert_rowid (db_handle);

    sqlite3_reset (stmt2);
    sqlite3_clear_bindings (stmt2);
    sqlite3_bind_text (stmt2, 1, group, strlen (group), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt2, 2, last_id);
    ret = sqlite3_step (stmt2);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt2, 0) == 1)
	      xret = 1;
      }
    if (xret != 1)
      {
	  fprintf (stderr, "Unable to register \"%s\"\n", path2);
	  *retcode += 4;
	  return 0;
      }

    sqlite3_reset (stmt1);
    sqlite3_clear_bindings (stmt1);
    sqlite3_bind_text (stmt1, 1, path2, strlen (path2), SQLITE_STATIC);
    ret = sqlite3_step (stmt1);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt1, 0) == 1)
	      xret = 1;
      }
    if (xret != 1)
      {
	  fprintf (stderr, "Unable to load \"%s\"\n", path2);
	  *retcode += 5;
	  return 0;
      }
    last_id = sqlite3_last_insert_rowid (db_handle);

    sqlite3_reset (stmt2);
    sqlite3_clear_bindings (stmt2);
    sqlite3_bind_text (stmt2, 1, group, strlen (group), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt2, 2, last_id);
    ret = sqlite3_step (stmt2);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt2, 0) == 1)
	      xret = 1;
      }
    if (xret != 1)
      {
	  fprintf (stderr, "Unable to register \"%s\"\n", path2);
	  *retcode += 6;
	  return 0;
      }
    sqlite3_finalize (stmt1);
    sqlite3_finalize (stmt2);

/* testing Group Style #1 */
    style =
	rl2_create_group_style_from_dbms (db_handle, "my_group",
					  "group_style_1");
    if (style == NULL)
      {
	  fprintf (stderr, "Unable to create Group Style #1\n");
	  *retcode += 7;
	  return 0;
      }
    str = rl2_get_group_style_name (style);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style Name #1\n");
	  *retcode += 8;
	  return 0;
      }
    if (strcmp (str, "group_style_1") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style Name #1: %s\n", str);
	  *retcode += 9;
	  return 0;
      }
    str = rl2_get_group_style_title (style);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style Title #1\n");
	  *retcode += 10;
	  return 0;
      }
    if (strcmp (str, "style-1 title") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style Title #1: %s\n", str);
	  *retcode += 11;
	  return 0;
      }
    str = rl2_get_group_style_abstract (style);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style Abstract #1\n");
	  *retcode += 12;
	  return 0;
      }
    if (strcmp (str, "style-1 abstract") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style Abstract #1: %s\n", str);
	  *retcode += 13;
	  return 0;
      }
    if (rl2_is_valid_group_style (style, &valid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group Style Validity #1\n");
	  *retcode += 14;
	  return 0;
      }
    if (valid != 1)
      {
	  fprintf (stderr, "Unexpected Group Style Validity #1: %d\n", valid);
	  *retcode += 15;
	  return 0;
      }
    if (rl2_get_group_style_count (style, &count) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group Style Count #1\n");
	  *retcode += 16;
	  return 0;
      }
    if (count != 2)
      {
	  fprintf (stderr, "Unexpected Group Style Count #1: %d\n", count);
	  *retcode += 17;
	  return 0;
      }
    str = rl2_get_group_named_layer (style, 1);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style NamedLayer #1\n");
	  *retcode += 18;
	  return 0;
      }
    if (strcmp (str, "dumb1") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style NamedLayer #1: %s\n", str);
	  *retcode += 19;
	  return 0;
      }
    str = rl2_get_group_named_style (style, 0);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style NamedLayer #1\n");
	  *retcode += 20;
	  return 0;
      }
    if (strcmp (str, "srtm_brightness") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style NamedStyle #1: %s\n", str);
	  *retcode += 21;
	  return 0;
      }
    if (rl2_is_valid_group_named_layer (style, 0, &valid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group NamedLayer Validity #1\n");
	  *retcode += 22;
	  return 0;
      }
    if (valid != 1)
      {
	  fprintf (stderr, "Unexpected Group NamedLayer Validity #1: %d\n",
		   valid);
	  *retcode += 23;
	  return 0;
      }
    if (rl2_is_valid_group_named_style (style, 1, &valid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group NamedStyle Validity #1\n");
	  *retcode += 24;
	  return 0;
      }
    if (valid != 1)
      {
	  fprintf (stderr, "Unexpected Group NamedStyle Validity #1: %d\n",
		   valid);
	  *retcode += 25;
	  return 0;
      }
    if (!test_group_renderer (db_handle, style, 1, retcode))
	return 0;
    rl2_destroy_group_style (style);

/* testing Group Style #2 */
    style =
	rl2_create_group_style_from_dbms (db_handle, "my_group",
					  "group_style_2");
    if (style == NULL)
      {
	  fprintf (stderr, "Unable to create Group Style #2\n");
	  *retcode += 26;
	  return 0;
      }
    str = rl2_get_group_style_name (style);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style Name #2\n");
	  *retcode += 27;
	  return 0;
      }
    if (strcmp (str, "group_style_2") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style Name #2: %s\n", str);
	  *retcode += 28;
	  return 0;
      }
    str = rl2_get_group_style_title (style);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style Title #2\n");
	  *retcode += 29;
	  return 0;
      }
    if (strcmp (str, "style-2 title") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style Title #2: %s\n", str);
	  *retcode += 30;
	  return 0;
      }
    str = rl2_get_group_style_abstract (style);
    if (str != NULL)
      {
	  fprintf (stderr, "Unexpected Group Style Abstract #2: %s\n", str);
	  *retcode += 31;
	  return 0;
      }
    if (rl2_is_valid_group_style (style, &valid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group Style Validity #2\n");
	  *retcode += 32;
	  return 0;
      }
    if (valid != 0)
      {
	  fprintf (stderr, "Unexpected Group Style Validity #2: %d\n", valid);
	  *retcode += 33;
	  return 0;
      }
    if (rl2_get_group_style_count (style, &count) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group Style Count #2\n");
	  *retcode += 34;
	  return 0;
      }
    if (count != 2)
      {
	  fprintf (stderr, "Unexpected Group Style Count #2: %d\n", count);
	  *retcode += 35;
	  return 0;
      }
    str = rl2_get_group_named_layer (style, 1);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style NamedLayer #2\n");
	  *retcode += 36;
	  return 0;
      }
    if (strcmp (str, "dumb0") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style NamedLayer #2: %s\n", str);
	  *retcode += 37;
	  return 0;
      }
    str = rl2_get_group_named_style (style, 1);
    if (str == NULL)
      {
	  fprintf (stderr, "Unable to get Group Style NamedLayer #2\n");
	  *retcode += 38;
	  return 0;
      }
    if (strcmp (str, "style1") != 0)
      {
	  fprintf (stderr, "Unexpected Group Style NamedStyle #2: %s\n", str);
	  *retcode += 39;
	  return 0;
      }
    if (rl2_is_valid_group_named_layer (style, 0, &valid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group NamedLayer Validity #2\n");
	  *retcode += 40;
	  return 0;
      }
    if (valid != 1)
      {
	  fprintf (stderr, "Unexpected Group NamedLayer Validity #2: %d\n",
		   valid);
	  *retcode += 41;
	  return 0;
      }
    if (rl2_is_valid_group_named_style (style, 1, &valid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Group NamedStyle Validity #2\n");
	  *retcode += 42;
	  return 0;
      }
    if (valid != 0)
      {
	  fprintf (stderr, "Unexpected Group NamedStyle Validity #2: %d\n",
		   valid);
	  *retcode += 43;
	  return 0;
      }
    if (!test_group_renderer (db_handle, style, 2, retcode))
	return 0;
    rl2_destroy_group_style (style);

/* testing NULL Group Style */
    style = rl2_create_group_style_from_dbms (db_handle, "lolly", "poppy");
    if (style != NULL)
      {
	  fprintf (stderr, "Unexpected success create Group Style\n");
	  *retcode += 44;
	  return 0;
      }
    rl2_get_group_style_name (NULL);
    rl2_get_group_style_title (NULL);
    rl2_get_group_style_abstract (NULL);
    rl2_is_valid_group_style (NULL, &valid);
    rl2_get_group_style_count (NULL, &count);
    rl2_get_group_named_layer (NULL, 1);
    rl2_get_group_named_style (NULL, 1);
    rl2_is_valid_group_named_layer (NULL, 0, &valid);
    rl2_is_valid_group_named_style (NULL, 1, &valid);

    return 1;
}

static int
load_symbolizer (sqlite3 * db_handle, const char *coverage, const char *path,
		 int no_web_connection, int *retcode)
{
/* loading a RasterSymbolizer as a RasterStyle */
    char *sql;
    int ret;
    int xret = 0;
    sqlite3_stmt *stmt;
    sqlite3_int64 last_id;

    if (no_web_connection)
	sql = "SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(?), 1))";
    else
	sql = "SELECT SE_RegisterRasterStyle(XB_Create(XB_LoadXML(?), 1, 1))";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Unable to create the SQL statement (Style)\n");
	  *retcode += 1;
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, path, strlen (path), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      xret = 1;
      }
    last_id = sqlite3_last_insert_rowid (db_handle);
    sqlite3_finalize (stmt);
    if (xret != 1)
      {
	  fprintf (stderr, "unable to register a Raster Style \"%s\")\n", path);
	  *retcode += 1;
	  return 0;
      }
    xret = 0;

    sql = "SELECT SE_RegisterRasterStyledLayer(?, ?)";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "Unable to create the SQL statement (RasterStyledLayer)\n");
	  *retcode += 1;
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, last_id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      xret = 1;
      }
    if (xret == 1)
	return 1;
    *retcode += 1;
    return 0;
}

static int
test_symbolizer_1 (sqlite3 * db_handle, const char *coverage,
		   const char *style_name, int *retcode)
{
/* testing a RasterSymbolizer */
    rl2RasterStylePtr style;
    const char *string;
    double value;
    int intval;
    unsigned char enhancement;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    style = rl2_create_raster_style_from_dbms (db_handle, coverage, style_name);
    if (style == NULL)
      {
	  fprintf (stderr, "Unable to retrieve style '%s'\n", style_name);
	  *retcode += 1;
	  return 0;
      }

    if (strcmp (coverage, "dumb1") == 0)
      {
	  string = rl2_get_raster_style_name (style);
	  if (string == NULL)
	    {
		fprintf (stderr, "Unexpected NULL style name (%s)\n",
			 style_name);
		*retcode += 1;
		return 0;
	    }
	  if (strcmp (string, "style1") != 0)
	    {
		fprintf (stderr, "Unexpected style name \"%s\"\n", string);
		*retcode += 2;
		return 0;
	    }

	  string = rl2_get_raster_style_title (style);
	  if (string == NULL)
	    {
		fprintf (stderr, "Unexpected NULL style title (%s)\n",
			 style_name);
		*retcode += 3;
		return 0;
	    }
	  if (strcmp (string, "title: first style") != 0)
	    {
		fprintf (stderr, "Unexpected style title \"%s\"\n", string);
		*retcode += 4;
		return 0;
	    }

	  string = rl2_get_raster_style_abstract (style);
	  if (string == NULL)
	    {
		fprintf (stderr, "Unexpected NULL style abstract (%s)\n",
			 style_name);
		*retcode += 5;
		return 0;
	    }
	  if (strcmp (string, "abstract: first style") != 0)
	    {
		fprintf (stderr, "Unexpected style abstract \"%s\"\n", string);
		*retcode += 6;
		return 0;
	    }
      }

    if (rl2_get_raster_style_opacity (style, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Opacity\n");
	  *retcode += 7;
	  return 0;
      }
    if (value != 1.0)
      {
	  fprintf (stderr, "Unexpected Opacity %1.2f\n", value);
	  *retcode += 8;
	  return 0;
      }

    if (rl2_is_raster_style_mono_band_selected (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get IsMonoBandSelected\n");
	  *retcode += 9;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected IsMonoBandSelected %d\n", intval);
	  *retcode += 10;
	  return 0;
      }

    if (rl2_is_raster_style_triple_band_selected (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get IsTripleBandSelected\n");
	  *retcode += 11;
	  return 0;
      }
    if (intval != 0)
      {
	  fprintf (stderr, "Unexpected IsTripleBandSelected %d\n", intval);
	  *retcode += 12;
	  return 0;
      }

    if (rl2_get_raster_style_overall_contrast_enhancement
	(style, &enhancement, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get OverallContrastEnhancement\n");
	  *retcode += 13;
	  return 0;
      }
    if (enhancement != RL2_CONTRAST_ENHANCEMENT_NONE)
      {
	  fprintf (stderr, "Unexpected OverallContrastEnhancement %02x\n",
		   enhancement);
	  *retcode += 14;
	  return 0;
      }

    if (rl2_has_raster_style_shaded_relief (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get HasShadedRelief\n");
	  *retcode += 15;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected HasShadedRelief %d\n", intval);
	  *retcode += 16;
	  return 0;
      }

    if (rl2_get_raster_style_shaded_relief (style, &intval, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ShadedRelief BrightnessOnly\n");
	  *retcode += 17;
	  return 0;
      }
    if (strcmp (coverage, "dumb1") == 0)
      {
	  if (intval != 0)
	    {
		fprintf (stderr, "Unexpected ShadedRelief BrightnessOnly %d\n",
			 intval);
		*retcode += 18;
		return 0;
	    }
	  if (value != 55.0)
	    {
		fprintf (stderr, "Unexpected ShadedRelief ReliefFactor %1.2f\n",
			 value);
		*retcode += 18;
		return 0;
	    }
      }
    else
      {
	  if (intval != 0)
	    {
		fprintf (stderr, "Unexpected ShadedRelief BrightnessOnly %d\n",
			 intval);
		*retcode += 18;
		return 0;
	    }
	  if (value != 33.5)
	    {
		fprintf (stderr, "Unexpected ShadedRelief ReliefFactor %1.2f\n",
			 value);
		*retcode += 18;
		return 0;
	    }
      }

    if (rl2_has_raster_style_color_map_interpolated (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get HasColorMapInterpolated\n");
	  *retcode += 19;
	  return 0;
      }
    if (intval != 0)
      {
	  fprintf (stderr, "Unexpected HasColorMapInterpolated %d\n", intval);
	  *retcode += 20;
	  return 0;
      }

    if (rl2_has_raster_style_color_map_categorized (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get HasColorMapCategorized\n");
	  *retcode += 21;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected HasColorMapCategorized %d\n", intval);
	  *retcode += 22;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_default (style, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapDefault\n");
	  *retcode += 23;
	  return 0;
      }
    if (red != 0x78 || green != 0xc8 || blue != 0x18)
      {
	  fprintf (stderr, "Unexpected ColorMapDefault #%02x%02x%02x\n", red,
		   green, blue);
	  *retcode += 24;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_category_base
	(style, &red, &green, &blue) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapCategoryBase\n");
	  *retcode += 25;
	  return 0;
      }
    if (red != 0x00 || green != 0xff || blue != 0x00)
      {
	  fprintf (stderr, "Unexpected ColorMapCategoryBase #%02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 26;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_count (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapCount\n");
	  *retcode += 27;
	  return 0;
      }
    if (intval != 19)
      {
	  fprintf (stderr, "Unexpected ColorMapCount %d\n", intval);
	  *retcode += 28;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_entry
	(style, 3, &value, &red, &green, &blue) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapEntry (3)\n");
	  *retcode += 29;
	  return 0;
      }
    if (value != -167)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (3) %1.2f\n", value);
	  *retcode += 30;
	  return 0;
      }
    if (red != 0x3c || green != 0xf5 || blue != 0x05)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (3) #%02x%02x%02x\n", red,
		   green, blue);
	  *retcode += 30;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_entry
	(style, 13, &value, &red, &green, &blue) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapEntry (13)\n");
	  *retcode += 31;
	  return 0;
      }
    if (value != 1400)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (13) %1.2f\n", value);
	  *retcode += 32;
	  return 0;
      }
    if (red != 0xc8 || green != 0x50 || blue != 0x00)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (13) #%02x%02x%02x\n", red,
		   green, blue);
	  *retcode += 33;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_entry
	(style, 30, &value, &red, &green, &blue) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: ColorMapEntry (30)\n");
	  *retcode += 34;
	  return 0;
      }

    if (rl2_get_raster_style_triple_band_selection (style, &red, &green, &blue)
	== RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TripleBand selection\n");
	  *retcode += 35;
	  return 0;
      }

    if (rl2_get_raster_style_mono_band_selection (style, &red) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get MonoBand selection\n");
	  *retcode += 36;
	  return 0;
      }
    if (red != 0)
      {
	  fprintf (stderr, "Unexpected MonoBand selection: %d\n", intval);
	  *retcode += 36;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_count (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapCount\n");
	  *retcode += 38;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_default (NULL, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapDefault\n");
	  *retcode += 39;
	  return 0;
      }

    if (rl2_get_raster_style_gray_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected GrayBandContrastEnhancement\n");
	  *retcode += 40;
	  return 0;
      }

    if (rl2_get_raster_style_red_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected RedBandContrastEnhancement\n");
	  *retcode += 41;
	  return 0;
      }

    if (rl2_get_raster_style_green_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected GreenBandContrastEnhancement\n");
	  *retcode += 42;
	  return 0;
      }

    if (rl2_get_raster_style_blue_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected BlueBandContrastEnhancement\n");
	  *retcode += 43;
	  return 0;
      }

    rl2_destroy_raster_style (style);
    return 1;
}

static int
test_symbolizer_2 (sqlite3 * db_handle, const char *coverage,
		   const char *style_name, int *retcode)
{
/* testing a RasterSymbolizer */
    rl2RasterStylePtr style;
    const char *string;
    double value;
    int intval;
    unsigned char enhancement;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    style = rl2_create_raster_style_from_dbms (db_handle, coverage, style_name);
    if (style == NULL)
      {
	  fprintf (stderr, "Unable to retrieve style '%s'\n", style_name);
	  *retcode += 1;
	  return 0;
      }

    if (strcmp (coverage, "dumb1") == 0)
      {
	  string = rl2_get_raster_style_name (style);
	  if (string == NULL)
	    {
		fprintf (stderr, "Unexpected NULL style name (%s)\n",
			 style_name);
		*retcode += 1;
		return 0;
	    }
	  if (strcmp (string, "style2") != 0)
	    {
		fprintf (stderr, "Unexpected style name \"%s\"\n", string);
		*retcode += 2;
		return 0;
	    }

	  string = rl2_get_raster_style_title (style);
	  if (string == NULL)
	    {
		fprintf (stderr, "Unexpected NULL style title (%s)\n",
			 style_name);
		*retcode += 3;
		return 0;
	    }
	  if (strcmp (string, "title: second style") != 0)
	    {
		fprintf (stderr, "Unexpected style title \"%s\"\n", string);
		*retcode += 4;
		return 0;
	    }

	  string = rl2_get_raster_style_abstract (style);
	  if (string != NULL)
	    {
		fprintf (stderr, "Unexpected NOT NULL style abstract (%s)\n",
			 style_name);
		*retcode += 5;
		return 0;
	    }
      }

    if (rl2_get_raster_style_opacity (style, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Opacity\n");
	  *retcode += 6;
	  return 0;
      }
    if (value != 0.8)
      {
	  fprintf (stderr, "Unexpected Opacity %1.2f\n", value);
	  *retcode += 7;
	  return 0;
      }

    if (rl2_is_raster_style_triple_band_selected (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get IsTripleBandSelected\n");
	  *retcode += 8;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected IsTripleBandSelected %d\n", intval);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_get_raster_style_red_band_contrast_enhancement
	(style, &enhancement, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RedBandContrastEnhancement\n");
	  *retcode += 10;
	  return 0;
      }
    if (enhancement != RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
      {
	  fprintf (stderr, "Unexpected RedBandContrastEnhancement %02x\n",
		   enhancement);
	  *retcode += 11;
	  return 0;
      }

    if (rl2_get_raster_style_green_band_contrast_enhancement
	(style, &enhancement, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GreenBandContrastEnhancement\n");
	  *retcode += 12;
	  return 0;
      }
    if (enhancement != RL2_CONTRAST_ENHANCEMENT_GAMMA)
      {
	  fprintf (stderr, "Unexpected GreenBandContrastEnhancement %02x\n",
		   enhancement);
	  *retcode += 13;
	  return 0;
      }
    if (value != 2.5)
      {
	  fprintf (stderr, "Unexpected GreenBandContrastEnhancement %1.2f\n",
		   value);
	  *retcode += 14;
	  return 0;
      }

    if (rl2_get_raster_style_blue_band_contrast_enhancement
	(style, &enhancement, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BlueBandContrastEnhancement\n");
	  *retcode += 15;
	  return 0;
      }
    if (enhancement != RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
      {
	  fprintf (stderr, "Unexpected BlueBandContrastEnhancement %02x\n",
		   enhancement);
	  *retcode += 16;
	  return 0;
      }

    if (rl2_get_raster_style_overall_contrast_enhancement
	(style, &enhancement, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get OverallContrastEnhancement\n");
	  *retcode += 17;
	  return 0;
      }
    if (enhancement != RL2_CONTRAST_ENHANCEMENT_GAMMA)
      {
	  fprintf (stderr, "Unexpected OverallContrastEnhancement %02x\n",
		   enhancement);
	  *retcode += 18;
	  return 0;
      }
    if (value != 1.2)
      {
	  fprintf (stderr, "Unexpected OverallContrastEnhancement %1.2f\n",
		   value);
	  *retcode += 19;
	  return 0;
      }

    if (rl2_has_raster_style_shaded_relief (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get HasShadedRelief\n");
	  *retcode += 20;
	  return 0;
      }
    if (intval != 0)
      {
	  fprintf (stderr, "Unexpected HasShadedRelief %d\n", intval);
	  *retcode += 21;
	  return 0;
      }

    if (rl2_get_raster_style_shaded_relief (style, &intval, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: ShadedRelief\n");
	  *retcode += 22;
	  return 0;
      }

    if (rl2_has_raster_style_color_map_interpolated (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get HasColorMapInterpolated\n");
	  *retcode += 23;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected HasColorMapInterpolated %d\n", intval);
	  *retcode += 24;
	  return 0;
      }

    if (rl2_has_raster_style_color_map_categorized (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get HasColorMapCategorized\n");
	  *retcode += 25;
	  return 0;
      }
    if (intval != 0)
      {
	  fprintf (stderr, "Unexpected HasColorMapCategorized %d\n", intval);
	  *retcode += 26;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_default (style, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapDefault\n");
	  *retcode += 27;
	  return 0;
      }
    if (red != 0xdd || green != 0xdd || blue != 0xdd)
      {
	  fprintf (stderr, "Unexpected ColorMapDefault #%02x%02x%02x\n", red,
		   green, blue);
	  *retcode += 28;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_category_base
	(style, &red, &green, &blue) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: get ColorMapCategoryBase\n");
	  *retcode += 29;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_count (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapCount\n");
	  *retcode += 30;
	  return 0;
      }
    if (intval != 2)
      {
	  fprintf (stderr, "Unexpected ColorMapCount %d\n", intval);
	  *retcode += 31;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_entry
	(style, 0, &value, &red, &green, &blue) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapEntry (0)\n");
	  *retcode += 32;
	  return 0;
      }
    if (value != 0)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (0) %1.2f\n", value);
	  *retcode += 33;
	  return 0;
      }
    if (red != 0x00 || green != 0x00 || blue != 0x00)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (0) #%02x%02x%02x\n", red,
		   green, blue);
	  *retcode += 34;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_entry
	(style, 1, &value, &red, &green, &blue) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ColorMapEntry (1)\n");
	  *retcode += 35;
	  return 0;
      }
    if (value != 255)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (1) %1.2f\n", value);
	  *retcode += 36;
	  return 0;
      }
    if (red != 0xff || green != 0xff || blue != 0xff)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry (1) #%02x%02x%02x\n", red,
		   green, blue);
	  *retcode += 37;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_entry
	(style, 2, &value, &red, &green, &blue) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: ColorMapEntry (2)\n");
	  *retcode += 38;
	  return 0;
      }

    if (rl2_get_raster_style_triple_band_selection (style, &red, &green, &blue)
	!= RL2_OK)
      {
	  fprintf (stderr, "Unable to get TripleBand selection\n");
	  *retcode += 35;
	  return 0;
      }
    if (red == 0 && green == 1 && blue == 2)
	;
    else
      {
	  fprintf (stderr, "Unexpected TripleBand %d,%d,%d\n", red, green,
		   blue);
	  *retcode += 36;
	  return 0;
      }

    if (rl2_get_raster_style_mono_band_selection (style, &red) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: MonoBand selection\n");
	  *retcode += 36;
	  return 0;
      }

    if (rl2_is_raster_style_mono_band_selected (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get IsMonoBandSelected\n");
	  *retcode += 37;
	  return 0;
      }
    if (intval != 0)
      {
	  fprintf (stderr, "Unexpected IsMonoBandSelected %d\n", intval);
	  *retcode += 38;
	  return 0;
      }

    if (rl2_get_raster_style_gray_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected GrayBandContrastEnhancement\n");
	  *retcode += 39;
	  return 0;
      }

    rl2_destroy_raster_style (style);
    return 1;
}

static int
test_symbolizer_3 (sqlite3 * db_handle, const char *coverage,
		   const char *style_name, int *retcode)
{
/* testing a RasterSymbolizer */
    rl2RasterStylePtr style;
    unsigned char enhancement;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int intval;
    double value;
    style = rl2_create_raster_style_from_dbms (db_handle, coverage, style_name);
    if (style == NULL)
      {
	  fprintf (stderr, "Unable to retrieve style '%s'\n", style_name);
	  *retcode += 1;
	  return 0;
      }

    if (rl2_get_raster_style_triple_band_selection (style, &red, &green, &blue)
	== RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TripleBand selection\n");
	  *retcode += 2;
	  return 0;
      }

    if (rl2_is_raster_style_mono_band_selected (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get IsMonoBandSelected\n");
	  *retcode += 3;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected IsMonoBandSelected %d\n", intval);
	  *retcode += 4;
	  return 0;
      }

    if (rl2_get_raster_style_mono_band_selection (style, &red) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get MonoBand selection\n");
	  *retcode += 5;
	  return 0;
      }
    if (red != 0)
      {
	  fprintf (stderr, "Unexpected MonoBand %d\n", red);
	  *retcode += 4;
	  return 0;
      }

    if (rl2_is_raster_style_triple_band_selected (style, &intval) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get IsTripleBandSelected\n");
	  *retcode += 5;
	  return 0;
      }
    if (intval != 0)
      {
	  fprintf (stderr, "Unexpected IsMonoBandSelected %d\n", intval);
	  *retcode += 6;
	  return 0;
      }

    if (rl2_get_raster_style_gray_band_contrast_enhancement
	(style, &enhancement, &value) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GrayBandContrastEnhancement\n");
	  *retcode += 7;
	  return 0;
      }
    if (enhancement != RL2_CONTRAST_ENHANCEMENT_NONE)
      {
	  fprintf (stderr, "Unexpected GrayBandContrastEnhancement %02x\n",
		   enhancement);
	  *retcode += 8;
	  return 0;
      }

    if (rl2_get_raster_style_green_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected GreenBandContrastEnhancement\n");
	  *retcode += 9;
	  return 0;
      }

    if (rl2_get_raster_style_blue_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected BlueBandContrastEnhancement\n");
	  *retcode += 10;
	  return 0;
      }

    if (rl2_get_raster_style_red_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected RedBandContrastEnhancement\n");
	  *retcode += 11;
	  return 0;
      }

    if (rl2_get_raster_style_green_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected GreenBandContrastEnhancement\n");
	  *retcode += 12;
	  return 0;
      }

    if (rl2_get_raster_style_blue_band_contrast_enhancement
	(style, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected BlueBandContrastEnhancement\n");
	  *retcode += 13;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_count (style, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapCount\n");
	  *retcode += 14;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_default (style, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapDefault\n");
	  *retcode += 15;
	  return 0;
      }

    rl2_destroy_raster_style (style);
    return 1;
}

static int
test_symbolizer_null (int *retcode)
{
/* testing a NULL RasterSymbolizer */
    const char *string;
    double value;
    int intval;
    unsigned char enhancement;
    unsigned char red;
    unsigned char green;
    unsigned char blue;

    string = rl2_get_raster_style_name (NULL);
    if (string != NULL)
      {
	  fprintf (stderr, "Unexpected style name\n");
	  *retcode += 1;
	  return 0;
      }

    string = rl2_get_raster_style_title (NULL);
    if (string != NULL)
      {
	  fprintf (stderr, "Unexpected style title\n");
	  *retcode += 2;
	  return 0;
      }

    string = rl2_get_raster_style_abstract (NULL);
    if (string != NULL)
      {
	  fprintf (stderr, "Unexpected style abstract\n");
	  *retcode += 3;
	  return 0;
      }

    if (rl2_get_raster_style_opacity (NULL, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected Opacity\n");
	  *retcode += 4;
	  return 0;
      }

    if (rl2_is_raster_style_mono_band_selected (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected IsMonoBandSelected\n");
	  *retcode += 5;
	  return 0;
      }

    if (rl2_is_raster_style_triple_band_selected (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected IsTripleBandSelected\n");
	  *retcode += 6;
	  return 0;
      }

    if (rl2_get_raster_style_overall_contrast_enhancement
	(NULL, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected OverallContrastEnhancement\n");
	  *retcode += 7;
	  return 0;
      }

    if (rl2_has_raster_style_shaded_relief (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected HasShadedRelief\n");
	  *retcode += 8;
	  return 0;
      }

    if (rl2_get_raster_style_shaded_relief (NULL, &intval, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected ShadedRelief BrightnessOnly\n");
	  *retcode += 9;
	  return 0;
      }

    if (rl2_has_raster_style_color_map_interpolated (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected HasColorMapInterpolated\n");
	  *retcode += 10;
	  return 0;
      }

    if (rl2_has_raster_style_color_map_categorized (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected HasColorMapCategorized\n");
	  *retcode += 11;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_default (NULL, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapDefault\n");
	  *retcode += 12;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_category_base
	(NULL, &red, &green, &blue) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapCategoryBase\n");
	  *retcode += 13;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_count (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapCount\n");
	  *retcode += 14;
	  return 0;
      }

    if (rl2_get_raster_style_color_map_entry
	(NULL, 3, &value, &red, &green, &blue) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected ColorMapEntry\n");
	  *retcode += 15;
	  return 0;
      }

    if (rl2_get_raster_style_triple_band_selection (NULL, &red, &green, &blue)
	== RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TripleBand selection\n");
	  *retcode += 16;
	  return 0;
      }

    if (rl2_get_raster_style_mono_band_selection (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: MonoBand selection\n");
	  *retcode += 17;
	  return 0;
      }

    if (rl2_get_raster_style_red_band_contrast_enhancement
	(NULL, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected RedBandContrastEnhancement\n");
	  *retcode += 18;
	  return 0;
      }

    if (rl2_get_raster_style_green_band_contrast_enhancement
	(NULL, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected GreenBandContrastEnhancement\n");
	  *retcode += 19;
	  return 0;
      }

    if (rl2_get_raster_style_blue_band_contrast_enhancement
	(NULL, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected BlueBandContrastEnhancement\n");
	  *retcode += 20;
	  return 0;
      }

    if (rl2_get_raster_style_gray_band_contrast_enhancement
	(NULL, &enhancement, &value) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected GrayBandContrastEnhancement\n");
	  *retcode += 21;
	  return 0;
      }

    rl2_destroy_raster_style (NULL);
    return 1;
}

int
main (int argc, char *argv[])
{
    int no_web_connection = 0;
    int result = 0;
    int ret;
    char *err_msg = NULL;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (getenv ("ENABLE_RL2_WEB_TESTS") == NULL)
      {
	  fprintf (stderr,
		   "this testcase has been executed with several limitations\n"
		   "because it was not enabled to access the Web.\n\n"
		   "you can enable all testcases requiring an Internet connection\n"
		   "by setting the environment variable \"ENABLE_RL2_WEB_TESTS=1\"\n\n");
	  no_web_connection = 1;
      }

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, 0);
    ret =
	sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT CreateRasterCoveragesTable()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -3;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT RL2_CreateCoverage('dumb1', 'UINT8', "
		      "'GRAYSCALE', 1, 'NONE', 100, 256, 256, 4326, 1.0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -4;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT RL2_CreateCoverage('dumb2', 'UINT8', "
		      "'GRAYSCALE', 1, 'NONE', 100, 256, 256, 4326, 1.0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -5;
      }
    ret =
	sqlite3_exec (db_handle,
		      "SELECT RL2_CreateCoverage('dumb_dem', 'INT16', "
		      "'DATAGRID', 1, 'NONE', 100, 256, 256, 4326, 1.0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -6;
      }
    if (no_web_connection)
	ret =
	    sqlite3_exec (db_handle, "SELECT CreateStylingTables(1)", NULL,
			  NULL, &err_msg);
    else
	ret =
	    sqlite3_exec (db_handle, "SELECT CreateStylingTables()", NULL,
			  NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateStylingTables() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -7;
      }
    ret =
	sqlite3_exec (db_handle,
		      "SELECT SE_RegisterStyledGroup('my_group', 'dumb1')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterStyledGroup() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -8;
      }
    ret =
	sqlite3_exec (db_handle,
		      "SELECT SE_RegisterStyledGroup('my_group', 'dumb_dem')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterStyledGroup() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -9;
      }

/* tests */
    ret = -100;
    if (!load_symbolizer
	(db_handle, "dumb1", "raster_symbolizer_1.xml", no_web_connection,
	 &ret))
	return ret;
    ret = -200;
    if (!load_symbolizer
	(db_handle, "dumb1", "raster_symbolizer_2.xml", no_web_connection,
	 &ret))
	return ret;
    ret = -300;
    if (!load_symbolizer
	(db_handle, "dumb1", "raster_symbolizer_3.xml", no_web_connection,
	 &ret))
	return ret;
    ret = -400;
    if (!load_symbolizer
	(db_handle, "dumb2", "raster_symbolizer_4.xml", no_web_connection,
	 &ret))
	return ret;
    ret = -500;
    if (!load_symbolizer
	(db_handle, "dumb2", "raster_symbolizer_5.xml", no_web_connection,
	 &ret))
	return ret;
    ret = -600;
    if (!load_symbolizer
	(db_handle, "dumb2", "raster_symbolizer_6.xml", no_web_connection,
	 &ret))
	return ret;
    ret = -800;
    if (!load_symbolizer
	(db_handle, "dumb_dem", "srtm_brightness.xml", no_web_connection, &ret))
	return ret;
    ret = -110;
    if (!test_symbolizer_1 (db_handle, "dumb1", "style1", &ret))
	return ret;
    ret = -210;
    if (!test_symbolizer_2 (db_handle, "dumb1", "style2", &ret))
	return ret;
    ret = -310;
    if (!test_symbolizer_3 (db_handle, "dumb1", "style3", &ret))
	return ret;
    ret = -410;
    if (!test_symbolizer_1 (db_handle, "dumb2", "style4", &ret))
	return ret;
    ret = -510;
    if (!test_symbolizer_2 (db_handle, "dumb2", "style5", &ret))
	return ret;
    ret = -610;
    if (!test_symbolizer_3 (db_handle, "dumb2", "style6", &ret))
	return ret;
    ret = -710;
    if (!test_symbolizer_null (&ret))
	return ret;
    ret = -810;
    if (!test_group_style (db_handle, no_web_connection, &ret))
	return -ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    if (old_SPATIALITE_SECURITY_ENV)
      {
#ifdef _WIN32
	  char *env = sqlite3_mprintf ("SPATIALITE_SECURITY=%s",
				       old_SPATIALITE_SECURITY_ENV);
	  putenv (env);
	  sqlite3_free (env);
#else /* not WIN32 */
	  setenv ("SPATIALITE_SECURITY", old_SPATIALITE_SECURITY_ENV, 1);
#endif
      }
    else
      {
#ifdef _WIN32
	  putenv ("SPATIALITE_SECURITY=");
#else /* not WIN32 */
	  unsetenv ("SPATIALITE_SECURITY");
#endif
      }
    return result;
}
