# - Try to find the WEBP library
#
# Once done this will define
#
#  WEBP_FOUND - System has libgta
#  WEBP_INCLUDE_DIR - The libgta include directory
#  WEBP_LIBRARIES - The libraries needed to use libgta


IF(WEBP_INCLUDE_DIR AND WEBP_LIBRARY)
    # in cache already
    SET(WEBP_FIND_QUIETLY TRUE)
ENDIF()

FIND_PACKAGE(PkgConfig QUIET)
IF(PKG_CONFIG_FOUND)
    # try using pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    PKG_CHECK_MODULES(PC_WEBP QUIET libwebp)
    SET(WEBP_VERSION_STRING ${PC_WEBP_VERSION})
ENDIF()

FIND_PATH(WEBP_INCLUDE_DIR webp/encode.h HINTS ${PC_WEBP_INCLUDE_DIRS})

FIND_LIBRARY(WEBP_LIBRARY NAMES webp libwebp HINTS ${PC_WEBP_LIBRARY_DIRS})

MARK_AS_ADVANCED(WEBP_INCLUDE_DIR WEBP_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set CFITSIO_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WEBP
    REQUIRED_VARS WEBP_LIBRARY WEBP_INCLUDE_DIR
    VERSION_VAR WEBP_VERSION_STRING
)

IF(WEBP_FOUND)
    SET(WEBP_LIBRARIES ${WEBP_LIBRARY})
    SET(WEBP_INCLUDE_DIRS ${WEBP_INCLUDE_DIR})
ENDIF()
