###############################################################################
# - Try to find Sqlite3
# Once done this will define
#
#  SQLITE3_FOUND - system has Sqlite3
#  SQLITE3_INCLUDE_DIRS - the Sqlite3 include directory
#  SQLITE3_LIBRARIES - Link these to use Sqlite3
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Copyright (c) 2016, NextGIS <info@nextgis.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
###############################################################################

if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
  # Already in cache, be silent
  set(SQLITE3_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    # try using pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    pkg_check_modules(PC_SQLITE3 QUIET sqlite3)
    SET(SQLITE3_VERSION_STRING ${PC_SQLITE3_VERSION} CACHE INTERNAL "")
endif()

find_path(SQLITE3_INCLUDE_DIR
          NAMES  sqlite3.h
          HINTS ${PC_SQLITE3_INCLUDE_DIR}
)

find_library(SQLITE3_LIBRARY
             NAMES sqlite3 sqlite3_i
             HINTS ${PC_SQLITE3_LIBRARY}
)
if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
    get_filename_component(SQLITE3_LIBRARY_DIR ${SQLITE3_LIBRARY} DIRECTORY)
        message(STATUS "${SQLITE3_LIBRARY_DIR}")
    find_PATH(SQLITE3_PCRE_LIBRARY
              NAMES pcre.${CMAKE_SHARED_LIBRARY_SUFFIX}
              SUFFIX_PATHS sqlite3
              PATHS /usr/lib
              HINTS ${SQLITE3_LIBRARY_DIR})
    # check column metadata
    set(SQLITE_COL_TEST_CODE "#ifdef __cplusplus
extern \"C\"
#endif
char sqlite3_column_table_name ();
int
main ()
{
return sqlite3_column_table_name ();
  return 0;
}
")
    check_c_source_compiles("${SQLITE_COL_TEST_CODE}"  SQLITE_HAS_COLUMN_METADATA)
    set(SQLITE_HAS_COLUMN_METADATA ${SQLITE_HAS_COLUMN_METADATA} CACHE BOOL "SQLite has column metadata.")
endif()
mark_as_advanced(SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR SQLITE3_PCRE_LIBRARY SQLITE_HAS_COLUMN_METADATA)

# Handle the QUIETLY and REQUIRED arguments and set GEOS_FOUND to TRUE
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQLITE3
                                  FOUND_VAR SQLITE3_FOUND
                                  REQUIRED_VARS SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR
                                  VERSION_VAR SQLITE3_VERSION_STRING)

if(SQLITE3_FOUND)
  set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
  set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
endif()
