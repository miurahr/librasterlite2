/*

 rl2sql -- main SQLite extension methods

 version 0.1, 2013 December 22

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

#define RL2_UNUSED() if (argc || argv) argc = argc;

static void
fnct_rl2_version (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_version()
/
/ return a text string representing the current RasterLite-2 version
*/
    int len;
    const char *p_result = rl2_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_target_cpu (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_target_cpu()
/
/ return a text string representing the current RasterLite-2 Target CPU
*/
    int len;
    const char *p_result = rl2_target_cpu ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
register_rl2_sql_functions (void *p_db)
{
    sqlite3 *db = p_db;
    sqlite3_create_function (db, "rl2_version", 0, SQLITE_ANY, 0,
			     fnct_rl2_version, 0, 0);
    sqlite3_create_function (db, "rl2_target_cpu", 0, SQLITE_ANY, 0,
			     fnct_rl2_target_cpu, 0, 0);
}

static void
rl2_splash_screen (int verbose)
{
    if (isatty (1))
      {
	  /* printing "hello" message only when stdout is on console */
	  if (verbose)
	    {
		printf ("RasterLite-2 version : %s\n", rl2_version ());
		printf ("TARGET CPU ..........: %s\n", rl2_target_cpu ());
	    }
      }
}

#ifndef LOADABLE_EXTENSION

RL2_DECLARE void
rl2_init (sqlite3 * handle, int verbose)
{
/* used when SQLite initializes as an ordinary lib */
    register_rl2_sql_functions (handle);
    rl2_splash_screen (verbose);
}

#else /* built as LOADABLE EXTENSION only */

SQLITE_EXTENSION_INIT1 static int
init_rl2_extension (sqlite3 * db, char **pzErrMsg,
		    const sqlite3_api_routines * pApi)
{
    SQLITE_EXTENSION_INIT2 (pApi);

    register_rl2_sql_functions (db);
    return 0;
}

#if !(defined _WIN32) || defined(__MINGW32__)
/* MSVC is unable to understand this declaration */
__attribute__ ((visibility ("default")))
#endif
     RL2_DECLARE int
	 sqlite3_rasterlite_init (sqlite3 * db, char **pzErrMsg,
				  const sqlite3_api_routines * pApi)
{
/* SQLite invokes this routine once when it dynamically loads the extension. */
    return init_rl2_extension (db, pzErrMsg, pApi);
}

#endif
