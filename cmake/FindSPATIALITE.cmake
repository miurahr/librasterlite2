# Find SpatiaLite
# ~~~~~~~~~~~~~~~
#
# Copyright (c) 2009, Sandro Furieri <a.furieri at lqt.it>
# Copyright (C) 2017, Hiroshi Miura
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# CMake module to search for SpatiaLite library
#
# If it's found it sets SPATIALITE_FOUND to TRUE
# and following variables are set:
#    SPATIALITE_INCLUDE_DIR
#    SPATIALITE_LIBRARY
#    SPATIALITE_VERSION_STRING

# FIND_PATH and FIND_LIBRARY normally search standard locations
# before the specified paths. To search non-standard paths first,
# FIND_* is invoked first with specified paths and NO_DEFAULT_PATH
# and then again with no specified paths to search the default
# locations. When an earlier FIND_* succeeds, subsequent FIND_*s
# searching for the same item do nothing. 

# try to use sqlite framework on mac
# want clean framework path, not unix compatibility path
IF (APPLE)
    IF (CMAKE_FIND_FRAMEWORK MATCHES "FIRST"
      OR CMAKE_FRAMEWORK_PATH MATCHES "ONLY"
      OR NOT CMAKE_FIND_FRAMEWORK)
        SET (CMAKE_FIND_FRAMEWORK_save ${CMAKE_FIND_FRAMEWORK} CACHE STRING "" FORCE)
        SET (CMAKE_FIND_FRAMEWORK "ONLY" CACHE STRING "" FORCE)
        FIND_PATH(SPATIALITE_INCLUDE_DIR spatialite.h)
        # if no spatialite header, we don't want sqlite find below to succeed
        IF (SPATIALITE_INCLUDE_DIR)
            FIND_LIBRARY(SPATIALITE_LIBRARY spatialite)
            # FIND_PATH doesn't add "Headers" for a framework
            SET (SPATIALITE_INCLUDE_DIR ${SPATIALITE_LIBRARY}/Headers CACHE PATH "Path to a file." FORCE)
        ENDIF (SPATIALITE_INCLUDE_DIR)
        SET (CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK_save} CACHE STRING "" FORCE)
    ENDIF ()
ENDIF (APPLE)

FIND_PACKAGE(PkgConfig QUIET)
IF(PKG_CONFIG_FOUND)
    # try using pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    PKG_CHECK_MODULES(PC_SPATIALITE QUIET spatialite)
    SET(SPATIALITE_VERSION_STRING ${PC_SPATIALITE_VERSION} CACHE INTERNAL "")
ENDIF()

FIND_PATH(SPATIALITE_INCLUDE_DIR
          NAMES spatialite.h
          HINSTS ${PC_SPATIALITE_INCLUDE_DIR})
FIND_LIBRARY(SPATIALITE_LIBRARY
             NAMES spatialite
             HINTS ${PC_SPATIALITE_LIBRARY})

mark_as_advanced(SPATIALITE_LIBRARY SPATIALITE_INCLUDE_DIR)

# Handle the QUIETLY and REQUIRED arguments and set GEOS_FOUND to TRUE
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SPATIALITE
                                  FOUND_VAR SPATIALITE_FOUND
                                  REQUIRED_VARS SPATIALITE_LIBRARY SPATIALITE_INCLUDE_DIR
                                  VERSION_VAR SPATIALITE_VERSION_STRING)

if(SPATIALITE_FOUND)
    set(SPATIALITE_LIBRARIES ${SPATIALITE_LIBRARY})
    set(SPATIALITE_INCLUDE_DIRS ${SPATIALITE_INCLUDE_DIR})
endif()

