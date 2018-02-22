## FindPIXMAN.cmake
##
## Copyright (C) 2016 Christian Schenk
## 
## This file is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published
## by the Free Software Foundation; either version 2, or (at your
## option) any later version.
## 
## This file is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this file; if not, write to the Free Software
## Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
## USA.

find_path(PIXMAN_INCLUDE_DIR
  NAMES
    pixman.h
  PATH_SUFFIXES
    pixman-1
)

find_library(PIXMAN_LIBRARY
  NAMES
    pixman-1
)

INCLUDE(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PIXMAN DEFAULT_MSG PIXMAN_LIBRARY PIXMAN_INCLUDE_DIR)

if(PIXMAN_FOUND)
  set(PIXMAN_INCLUDE_DIRS ${PIXMAN_INCLUDE_DIR})
  set(PIXMAN_LIBRARIES ${PIXMAN_LIBRARY})
else()
  set(PIXMAN_INCLUDE_DIRS)
  set(PIXMAN_LIBRARIES)
endif()

mark_as_advanced(PIXMAN_LIBRARY PIXMAN_INCLUDE_DIR)
