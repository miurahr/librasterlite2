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
        ${FREETYPE_INCLUDE_DIRS}
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
        ${CURL_LIBRARY}
        ${FREETYPE_LIBRARY}
        ${FREEXL_LIBRARY}
        ${GEOS_LIBRARIES}
        ${GEOTIFF_LIBRARIES}
        ${GIFLIB_LIBRARY}
        ${JPEG_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${PNG_LIBRARIES}
        ${PIXMAN_LIBRARIES}
        ${PROJ4_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        ${SPATIALITE_LIBRARIES}
        ${ZLIB_LIBRARIES})

find_package(Iconv)
if(ICONV_FOUND)
  if(NOT Iconv_IS_BUILT_IN)
    target_include_directories(${TARGET_NAME} PRIVATE ${Iconv_INCLUDE_DIR})
    target_link_libraries(${TARGET_NAME} PRIVATE ${Iconv_LIBRARY})
  endif()
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE "-DVERSION=\"${PROJECT_VERSION}\"")
target_compile_definitions(${TARGET_NAME} PRIVATE "-DTARGET_CPU=\"${CMAKE_SYSTEM_PROCESSOR}\"")
if(MSVC)
  target_compile_definitions(${TARGET_NAME} PRIVATE "-DM_PI=3.14159265358979323846")
endif()

if(CHARLS_FOUND AND CHARLS_VERSION EQUAL 1)
  target_link_libraries(${TARGET_NAME} PUBLIC ${CHARLS_LIBRARIES})
else()
  target_compile_definitions(${TARGET_NAME} PRIVATE -DOMIT_CHARLS)
endif()
if(LZMA_FOUND)
  target_link_libraries(${TARGET_NAME} PUBLIC ${LZMA_LIBRARY})
else()
  target_compile_definitions(${TARGET_NAME} PRIVATE -DOMIT_LZMA)
endif()
if(OPENJPEG_FOUND)
  target_link_libraries(${TARGET_NAME} PUBLIC ${OPENJPEG_LIBRARIES})
  if(OPENJPEG_VERSION_NUM GREATER_EQUAL 20100)
    if(OPENJPEG_VERSION_NUM GREATER_EQUAL 20200)
      if(OPENJPEG_VERSION_NUM GREATER_EQUAL 20300)
        add_definitions(-DOPENJPEG_2_3)
      else()
        add_definitions(-DOPENJPEG_2_2)
      endif()
    else()
      add_definitions(-DOPENJPEG_2_1)
    endif()
  else()
    message(FATAL_ERROR "Unsupported openjpeg library version: ${OPENJPEG_VERSION_STRING}")
  endif()
else()
  target_compile_definitions(${TARGET_NAME} PRIVATE -DOMIT_OPENJPEG)
endif()
if(WEBP_FOUND)
  target_link_libraries(${TARGET_NAME} PUBLIC ${WEBP_LIBRARY})
else()
  target_compile_definitions(${TARGET_NAME} PRIVATE -DOMIT_WEBP)
endif()


install(TARGETS ${TARGET_NAME}
            EXPORT ${TARGET_NAME}-depends
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/headers/rasterlite2
            DESTINATION include)
