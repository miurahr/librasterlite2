# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindIconv
---------

This module finds the ``iconv()`` POSIX.1 functions on the system.
These functions might be provided in the regular C library or externally
in the form of an additional library.

The following variables are provided to indicate iconv support:

.. variable:: Iconv_FOUND

  Variable indicating if the iconv support was found.

.. variable:: Iconv_INCLUDE_DIRS

  The directories containing the iconv headers.

.. variable:: Iconv_LIBRARIES

  The iconv libraries to be linked.

.. variable:: Iconv_IS_BUILT_IN

  A variable indicating whether iconv support is stemming from the
  C library or not. Even if the C library provides `iconv()`, the presence of
  an external `libiconv` implementation might lead to this being false.

.. variable:: ICONV_CONST

  A variable indicating whether iconv second argument is "const" or not.
  If iconv second arugment type is "const char**" then ICONV_CONST is a string "const",
  otherwise it is "".

.. variable:: ICONV_SECOND_ARGUMENT_IS_CONST

  A variable indicating same as Iconv_CONST but is boolean.
  If iconv second argument type is "const char**", it is ON(TRUE),
  otherwise OFF(FALSE).

Additionally, the following :prop_tgt:`IMPORTED` target is being provided:

.. variable:: Iconv::Iconv

  Imported target for using iconv.

The following cache variables may also be set:

.. variable:: Iconv_INCLUDE_DIR

  The directory containing the iconv headers.

.. variable:: Iconv_LIBRARY

  The iconv library (if not implicitly given in the C library).

.. note::
  On POSIX platforms, iconv might be part of the C library and the cache
  variables ``Iconv_INCLUDE_DIR`` and ``Iconv_LIBRARY`` might be empty.

#]=======================================================================]

include(CMakePushCheckState)
if(CMAKE_C_COMPILER_LOADED)
  include(CheckCSourceCompiles)
elseif(CMAKE_CXX_COMPILER_LOADED)
  include(CheckCXXSourceCompiles)
endif()
if(NOT CMAKE_C_COMPILER_LOADED AND NOT CMAKE_CXX_COMPILER_LOADED)
  # If neither C nor CXX are loaded, implicit iconv makes no sense.
  set(Iconv_IS_BUILT_IN FALSE)
endif()

# iconv can only be provided in libc on a POSIX system.
# If any cache variable is already set, we'll skip this test.
if(NOT DEFINED Iconv_IS_BUILT_IN)
  if(UNIX AND NOT DEFINED Iconv_INCLUDE_DIR AND NOT DEFINED Iconv_LIBRARY)
    cmake_push_check_state(RESET)
    # We always suppress the message here: Otherwise on supported systems
    # not having iconv in their C library (e.g. those using libiconv)
    # would always display a confusing "Looking for iconv - not found" message
    set(CMAKE_FIND_QUIETLY TRUE)
    # The following code will not work, but it's sufficient to see if it compiles.
    # Note: libiconv will define the iconv functions as macros, so CheckSymbolExists
    # will not yield correct results.
    set(Iconv_IMPLICIT_TEST_CODE
        "
      #include <stddef.h>
      #include <iconv.h>
      int main() {
        char *a, *b;
        size_t i, j;
        iconv_t ic;
        ic = iconv_open(\"to\", \"from\");
        iconv(ic, &a, &i, &b, &j);
        iconv_close(ic);
      }
      "
        )
    if(CMAKE_C_COMPILER_LOADED)
      check_c_source_compiles("${Iconv_IMPLICIT_TEST_CODE}" Iconv_IS_BUILT_IN)
    else()
      check_cxx_source_compiles("${Iconv_IMPLICIT_TEST_CODE}" Iconv_IS_BUILT_IN)
    endif()
    cmake_pop_check_state()
  else()
    set(Iconv_IS_BUILT_IN FALSE)
  endif()
endif()

if(NOT Iconv_IS_BUILT_IN)
  FIND_PATH(Iconv_INCLUDE_DIR iconv.h PATHS /opt/local/include NO_DEFAULT_PATH)
  find_path(Iconv_INCLUDE_DIR
            NAMES "iconv.h"
            DOC "iconv include directory")
  set(Iconv_LIBRARY_NAMES "iconv" "libiconv")
  FIND_LIBRARY(iconv_lib NAMES iconv libiconv NO_DEFAULT_PATH PATHS /opt/local/lib)
else()
  set(Iconv_INCLUDE_DIR "" CACHE FILEPATH "iconv include directory")
  set(Iconv_LIBRARY_NAMES "c")
endif()

find_library(Iconv_LIBRARY
             NAMES ${Iconv_LIBRARY_NAMES}
             DOC "iconv library (potentially the C library)")

mark_as_advanced(Iconv_INCLUDE_DIR)
mark_as_advanced(Iconv_LIBRARY)

include(FindPackageHandleStandardArgs)
if(NOT Iconv_IS_BUILT_IN)
  find_package_handle_standard_args(Iconv REQUIRED_VARS Iconv_LIBRARY Iconv_INCLUDE_DIR)
else()
  find_package_handle_standard_args(Iconv REQUIRED_VARS Iconv_LIBRARY)
endif()

IF(Iconv_FOUND)
  set (CMAKE_C_FLAGS_BACKUP "${CMAKE_C_FLAGS}")
  set (CMAKE_CXX_FLAGS_BACKUP "${CMAKE_CXX_FLAGS}")
  if(MSVC)
      set(CMAKE_C_FLAGS "${CMKAE_C_FLAGS} /WX")
      set(CMAKE_CXX_FLAGS "${CMKAE_CXX_FLAGS} /WX")
  else()
      include(CheckCCompilerFlag)
      check_c_compiler_flag("-Werror" ICONV_HAVE_WERROR)
      if(ICONV_HAVE_WERROR)
          set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
          set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
      endif(ICONV_HAVE_WERROR)
  endif()
  set(CMAKE_REQUIRED_INCLUDES ${Iconv_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${Iconv_LIBRARY})
  set(ICONV_CONST_TEST_CODE "#include <stdlib.h>
    #include <iconv.h>
    #ifdef __cplusplus
    extern \"C\"
    #endif

    int main(){
    #if defined(__STDC__) || defined(__cplusplus)
      iconv_t conv = 0;
      char* in = 0;
      size_t ilen = 0;
      char* out = 0;
      size_t olen = 0;
      size_t ret = iconv(conv, &in, &ilen, &out, &olen);
    #else
      size_t ret = iconv();
    #endif
      return 0;
    }")
  if(CMAKE_C_COMPILER_LOADED)
    check_c_source_compiles("${ICONV_CONST_TEST_CODE}"
                            _ICONV_SECOND_ARGUMENT_IS_NOT_CONST)
  elseif(CMAKE_CXX_COMPILER_LOADED)
    check_cxx_source_compiles("${ICONV_CONST_TEST_CODE}"
                              _ICONV_SECOND_ARGUMENT_IS_NOT_CONST)
  endif()
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_BACKUP}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_BACKUP}")
  if(_ICONV_SECOND_ARGUMENT_IS_NOT_CONST)
    set(ICONV_CONST "")
    set(ICONV_CPP_CONST "")
    set(ICONV_SECOND_ARGUMENT_IS_CONST OFF)
  else()
    set(ICONV_CONST "const")
    set(ICONV_CPP_CONST "const")
    set(ICONV_SECOND_ARGUMENT_IS_CONST ON)
  endif()
  unset(_ICONV_SECOND_ARGUMENT_IS_NOT_CONST)

  set(Iconv_INCLUDE_DIRS "${Iconv_INCLUDE_DIR}")
  set(Iconv_LIBRARIES "${Iconv_LIBRARY}")
  if(NOT TARGET Iconv::Iconv)
    add_library(Iconv::Iconv INTERFACE IMPORTED)
  endif()
  set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${Iconv_INCLUDE_DIRS}")
  set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_LINK_LIBRARIES "${Iconv_LIBRARIES}")
endif()
