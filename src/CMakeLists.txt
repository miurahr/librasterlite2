if (MSVC OR CMAKE_CONFIGURATION_TYPES)
  # For multi-config systems and for Visual Studio, the debug versions
  # of the libraries have a _d suffix.
  set (CMAKE_DEBUG_POSTFIX _d)
endif ()
if(MSVC)
  set(EXPORT_SYMBOL DLL_EXPORT)
else()
  set(EXPORT_SYMBOL EXPORT)
endif()

file(GLOB C_SOURCES *.c)
add_library(${TARGET_NAME} ${C_SOURCES})
if(BUILD_SHARED_LIBS)
    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${LIB_NAME}"
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        VERSION   ${PROJECT_VERSION}
        CLEAN_DIRECT_OUTPUT 1
        DEFINE_SYMBOL ${EXPORT_SYMBOL})
else()
    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${LIB_NAME}"
        VERSION   ${PROJECT_VERSION}
        CLEAN_DIRECT_OUTPUT 1)
endif()
target_include_directories(${TARGET_NAME}
      PRIVATE
        ${HEADER_DIRS}
      PUBLIC
        ${CAIRO_INCLUDE_DIR}
        ${CHARLS_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${FREEXL_INCLUDE_DIR}
        ${GEOS_INCLUDE_DIRS}
        ${GEOTIFF_INCLUDE_DIRS}
        ${GIFLIB_INCLUDE_DIR}
        ${JPEG_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${OPENJPEG_INCLUDE_DIR}
        ${PIXMAN_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIRS}
        ${PROJ4_INCLUDE_DIRS}
        ${SPATIALITE_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS}
        ${TIFF_INCLUDE_DIRS}
        ${WEBP_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PUBLIC
        ${TIFF_LIBRARIES}
        ${CAIRO_LIBRARY}
        ${CHARLS_LIBRARIES}
        ${CURL_LIBRARY}
        ${FREEXL_LIBRARY}
        ${GEOS_LIBRARIES}
        ${GEOTIFF_LIBRARIES}
        ${GIFLIB_LIBRARY}
        ${JPEG_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${LZMA_LIBRARY}
        ${OPENJPEG_LIBRARIES}
        ${PNG_LIBRARIES}
        ${PIXMAN_LIBRARIES}
        ${PROJ4_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        ${SPATIALITE_LIBRARIES}
        ${WEBP_LIBRARIES}
        ${ZLIB_LIBRARIES})

find_package(Iconv)
if(ICONV_FOUND)
  if(NOT Iconv_IS_BUILT_IN)
    target_include_directories(${TARGET_NAME} PRIVATE ${Iconv_INCLUDE_DIR})
    target_link_libraries(${TARGET_NAME} PRIVATE ${Iconv_LIBRARY})
  endif()
endif()

add_definitions("-DVERSION=\"${PROJECT_VERSION}\"")
add_definitions("-DTARGET_CPU=\"${CMAKE_SYSTEM_PROCESSOR}\"")
if(MSVC)
add_definitions("-DM_PI=3.14159265358979323846")
endif()

install(TARGETS ${TARGET_NAME}
            EXPORT ${TARGET_NAME}-depends
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/headers/rasterlite2
            DESTINATION include)
