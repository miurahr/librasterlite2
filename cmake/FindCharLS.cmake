#.rst:
# FindCharLS - JPEG Loss-Less Open SOurce Library CharLS
# --------
#
# Find CharLS
#
# ::
#
#   CHARLS_INCLUDE_DIR, where to find charls.h, etc.
#   CHARLS_LIBRARIES, the libraries needed to use CharLS.
#   CHARLS_FOUND, If false, do not try to use CharLS.
#   CHARLS_VERION, 1 if CharLS/interface.h exist and 2 if CharLS/charls.h exist
#

find_path(CHARLS_INCLUDE_DIR NAMES charls.h SUFFIX_PATHS CharLS)
find_path(CHARLS_INCLUDE_DIR NAMES interface.h SUFFIX_PATHS CharLS)

if(CHARLS_INCLUDE_DIR)
    if(EXISTS "${CHARLS_INCLUDE_DIR}/CharLS/interface.h")
        set(CHARLS_VERSION 1)
    else()
        set(CHARLS_VERSION 2)
    endif()
endif()

find_library(CHARLS_LIBRARY NAMES CharLS)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CHARLS
                                  FOUND_VAR CHARLS_FOUND
                                  REQUIRED_VARS CHARLS_LIBRARY CHARLS_INCLUDE_DIR
                                  VERSION_VAR CHARLS_VERSION)
mark_as_advanced(CHARLS_LIBRARY CHARLS_INCLUDE_DIR CHARLS_VERSION)

if(CHARLS_FOUND)
    set(CHARLS_LIBRARIES ${CHARLS_LIBRARY})
    set(CHARLS_INCLUDE_DIRS ${CHARLS_INCLUDE_DIRS})
endif()