# File found at: https://svn.osgeo.org/ossim/trunk/ossim_package_support/cmake/CMakeModules/FindOpenJPEG.cmake
# Modified for NextGIS Borsch project.

###
# File:  FindOpenJPEG.cmake
#
# Original script was from:
# http://code.google.com/p/emeraldviewer/source/browse/indra/cmake
###

function(TRANSFORM_VERSION _numerical_result _version_major _version_minor _version_patch)
  set(factor 100)
  if(_version_minor GREATER 99)
      set(factor 1000)
  endif()
  if(_verion_patch GREATER 99)
      set(factor 1000)
  endif()
  math(EXPR _internal_numerical_result
          "${major}*${factor}*${factor} + ${minor}*${factor} + ${patch}"
          )
  set(${_numerical_result} ${_internal_numerical_result} PARENT_SCOPE)
endfunction(TRANSFORM_VERSION)


# - Find OpenJPEG
# Find the OpenJPEG includes and library
# This module defines
#  OPENJPEG_INCLUDE_DIR, where to find openjpeg.h, etc.
#  OPENJPEG_LIBRARIES, the libraries needed to use OpenJPEG.
#  OPENJPEG_FOUND, If false, do not try to use OpenJPEG.
# also defined, but not for general use are
#  OPENJPEG_LIBRARY, where to find the OpenJPEG library.

FIND_PATH(OPENJPEG_INCLUDE_DIR opj_config.h
  PATHS
    /usr/local/include
    /usr/include
  PATH_SUFFIXES
    openjpeg-2.0
    openjpeg-2.1
    openjpeg-2.2
    openjpeg-2.3
  DOC "Location of OpenJPEG Headers"
)

SET(OPENJPEG_NAMES ${OPENJPEG_NAMES} openjp2)
FIND_LIBRARY(OPENJPEG_LIBRARY
  NAMES ${OPENJPEG_NAMES}
  PATHS /usr/lib /usr/local/lib
  )
MARK_AS_ADVANCED(OPENJPEG_LIBRARY OPENJPEG_INCLUDE_DIR)

if(OPENJPEG_INCLUDE_DIR)
    set(MAJOR_VERSION 0)
    set(MINOR_VERSION 0)
    set(REV_VERSION 0)

    if(EXISTS "${OPENJPEG_INCLUDE_DIR}/opj_config.h")
        file(READ "${OPENJPEG_INCLUDE_DIR}/opj_config.h" VERSION_H_CONTENTS)
        string(REGEX MATCH "OPJ_VERSION_MAJOR[ \t]+([0-9]+)"
          MAJOR_VERSION ${VERSION_H_CONTENTS})
        string (REGEX MATCH "([0-9]+)" MAJOR_VERSION ${MAJOR_VERSION})
        string(REGEX MATCH "OPJ_VERSION_MINOR[ \t]+([0-9]+)"
          MINOR_VERSION ${VERSION_H_CONTENTS})
        string (REGEX MATCH "([0-9]+)"
          MINOR_VERSION ${MINOR_VERSION})
        string(REGEX MATCH "OPJ_VERSION_BUILD[ \t]+([0-9]+)"
          REV_VERSION ${VERSION_H_CONTENTS})
        string (REGEX MATCH "([0-9]+)"
          REV_VERSION ${REV_VERSION})
        unset(VERSION_H_CONTENTS)
    endif()

    set(factor 100)
    set(OPENJPEG_VERSION_STRING "${MAJOR_VERSION}.${MINOR_VERSION}.${REV_VERSION}")
    math(EXPR OPENJPEG_VERSION_NUM
          "${MAJOR_VERSION}*${factor}*${factor} + ${MINOR_VERSION}*${factor} + ${REV_VERSION}"
          )
    unset(factor)
    unset(MAJOR_VERSION)
    unset(MINOR_VERSION)
    unset(REV_VERSION)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OPENJPEG 
                                  REQUIRED_VARS OPENJPEG_LIBRARY OPENJPEG_INCLUDE_DIR 
                                  VERSION_VAR OPENJPEG_VERSION_STRING)

IF(OPENJPEG_FOUND)
  set(OPENJPEG_LIBRARIES ${OPENJPEG_LIBRARY})
  set(OPENJPEG_INCLUDE_DIRS ${OPENJPEG_INCLUDE_DIR})
ENDIF()

