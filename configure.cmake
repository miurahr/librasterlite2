
# Include all the necessary files for macros
include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckLibraryExists)
include (CheckSymbolExists)
include (CheckTypeSize)
include (TestBigEndian)
include (CheckCSourceCompiles)

check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(getcwd HAVE_GETCWD)
check_function_exists(sqrt HAVE_SQRT)

check_include_file("dlfch.h" HAVE_DLFCH_H)
check_include_file("float.h" HAVE_FLOAT_H)

check_include_file("geotiff/geotiff.h" HAVE_GEOTIFF_GEOTIFF_H)
check_include_file("geotiff.h" HAVE_GEOTIFF_H)
check_include_file("gif_lib.h" HAVE_GIF_LIB_H)
check_include_file("openjpeg-2.0/openjpeg.h" HAVE_OPENJPEG_2_0_OPENJPEG_H)
check_include_file("openjpeg-2.1/openjpeg.h" HAVE_OPENJPEG_2_1_OPENJPEG_H)
check_include_file("openjpeg-2.2/openjpeg.h" HAVE_OPENJPEG_2_2_OPENJPEG_H)
check_include_file("openjpeg-2.3/openjpeg.h" HAVE_OPENJPEG_2_3_OPENJPEG_H)

check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("jerror.h" HAVE_JERROR_H)
check_include_file("jpeglib.h" HAVE_JPEGLIB_H)
check_include_file("lzma.h" HAVE_LZMA_H)
check_include_file("math.h" HAVE_MATH_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("png.h" HAVE_PNG_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdio.h" HAVE_STDIO_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)

check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strftime HAVE_STRFTIME)

check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("string.h" HAVE_STRING_H)

check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(strstr HAVE_STRSTR)

check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/unistd.h" HAVE_SYS_UNISTD_H)
check_include_file("zlib.h" HAVE_ZLIB_H)
