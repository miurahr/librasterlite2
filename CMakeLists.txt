cmake_minimum_required(VERSION 3.5)
project(librasterlite2
        VERSION 1.0.0.0
        LANGUAGES C)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(TARGET_NAME rasterlite2-target)
set(LIB_NAME    rasterlite2)

option(BUILD_SHARED_LIBS "Build shared library." ON)
if(MSVC)
    if(BUILD_SHARED_LIBS)
        set(CL_FLAGS_DBG "/MDd /Zi")
        set(CL_FLAGS_REL "/MD /Ox")
    else()
        set(CL_FLAGS_DBG "/MTd /Zi")
        set(CL_FLAGS_REL "/MT /Ox")
    endif()
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /nologo ${CL_FLAGS_REL}")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /nologo ${CL_FLAGS_DBG}")
endif(MSVC)

set(HEADER_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# macro
include(FindPackageHandleStandardArgs)
macro(local_find_package pkgname include_file library_file)
  find_path(${pkgname}_INCLUDE_DIR ${include_file})
  find_library(${pkgname}_LIBRARY ${library_file})
  find_package_handle_standard_args(${pkgname}
      REQUIRED_VARS ${pkgname}_INCLUDE_DIR ${pkgname}_LIBRARY)
endmacro()

#Find dependencies
local_find_package(CAIRO "cairo/cairo.h" "cairo")
find_package(CharLS REQUIRED)
find_package(CURL REQUIRED)
local_find_package(FREEXL "freexl.h" "freexl")
find_package(GEOS REQUIRED)
find_package(GeoTIFF REQUIRED)
local_find_package(GIFLIB "gif_lib.h" "gif")
find_package(Iconv)
find_package(JPEG REQUIRED)
find_package(LibXml2 REQUIRED)
local_find_package(LZMA "lzma/version.h" "lzma")
find_package(OpenJPEG2)
find_package(Pixman REQUIRED)
find_package(PNG REQUIRED)
find_package(PROJ4 REQUIRED)
find_package(SPATIALITE REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(TIFF REQUIRED)
find_package(WEBP REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads)

# build library
add_subdirectory(src)
