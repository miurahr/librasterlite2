# Find GEOS
# ~~~~~~~~~
# Copyright (C) 2017, Hiroshi Miura
# Copyright (c) 2008, Mateusz Loskot <mateusz@loskot.net>
# (based on FindGDAL.cmake by Magnus Homann)
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# CMake module to search for GEOS library
#
# If it's found it sets GEOS_FOUND to TRUE
# and following variables are set:
#    GEOS_INCLUDE_DIR
#    GEOS_LIBRARY
#

# try to use framework on mac
# want clean framework path, not unix compatibility path
IF (APPLE)
    IF (CMAKE_FIND_FRAMEWORK MATCHES "FIRST"
        OR CMAKE_FRAMEWORK_PATH MATCHES "ONLY"
        OR NOT CMAKE_FIND_FRAMEWORK)
        SET (CMAKE_FIND_FRAMEWORK_save ${CMAKE_FIND_FRAMEWORK} CACHE STRING "" FORCE)
        SET (CMAKE_FIND_FRAMEWORK "ONLY" CACHE STRING "" FORCE)
        FIND_LIBRARY(GEOS_LIBRARY GEOS)
        IF (GEOS_LIBRARY)
            # they're all the same in a framework
            SET (GEOS_INCLUDE_DIR ${GEOS_LIBRARY}/Headers CACHE PATH "Path to a file.")
            # set GEOS_CONFIG to make later test happy, not used here, may not exist
            SET (GEOS_CONFIG ${GEOS_LIBRARY}/unix/bin/geos-config CACHE FILEPATH "Path to a program.")
            # version in info.plist
            GET_VERSION_PLIST (${GEOS_LIBRARY}/Resources/Info.plist GEOS_VERSION)
            IF (NOT GEOS_VERSION)
                MESSAGE (FATAL_ERROR "Could not determine GEOS version from framework.")
            ENDIF (NOT GEOS_VERSION)
            STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" GEOS_VERSION_MAJOR "${GEOS_VERSION}")
            STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" GEOS_VERSION_MINOR "${GEOS_VERSION}")
            IF (GEOS_VERSION_MAJOR LESS 3)
                MESSAGE (FATAL_ERROR "GEOS version is too old (${GEOS_VERSION}). Use 3.0.0 or higher.")
            ENDIF (GEOS_VERSION_MAJOR LESS 3)
        ENDIF (GEOS_LIBRARY)
        SET (CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK_save} CACHE STRING "" FORCE)
    ENDIF ()
ENDIF (APPLE)

find_program(GEOS_CONFIG geos-config)
if(GEOS_CONFIG)
    EXEC_PROGRAM(${GEOS_CONFIG}
                 ARGS --version
                 OUTPUT_VARIABLE GEOS_VERSION)
    EXEC_PROGRAM(${GEOS_CONFIG}
                 ARGS --prefix
                 OUTPUT_VARIABLE GEOS_PREFIX)
endif()

FIND_PATH(GEOS_INCLUDE_DIR
          NAMES geos_c.h
          HINSTS ${GEOS_PREFIX}/include)
FIND_LIBRARY(GEOS_LIBRARY
             NAMES geos_c
             HINTS ${GEOS_PREFIX}/lib)

mark_as_advanced(GEOS_INCLUDE_DIR GEOS_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GEOS FOUND_VAR GEOS_FOUND REQUIRED_VARS GEOS_INCLUDE_DIR GEOS_LIBRARY)

if(GEOS_FOUND)
    set(GEOS_LIBRARIES ${GEOS_LIBRARY})
    set(GEOS_INCLUDE_DIRS ${GEOS_INCLUDE_DIR})
endif()