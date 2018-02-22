if(MSVC)
    set(UTIL_TARGET rl2tool)
else()
    set(UTIL_TARGET rl2tool wmslite)
endif()
foreach(UTILNAME IN LISTS UTIL_TARGET)
    add_executable(${UTILNAME} ${UTILNAME}.c)
    target_include_directories(${UTILNAME} PRIVATE ${HEADER_DIRS}
        ${SPATIALITE_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS})
    target_link_libraries(${UTILNAME} PRIVATE $<TARGET_NAME:${TARGET_NAME}>
        ${SPATIALITE_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        Threads::Threads
        -lm)
    if(NOT BUILD_SHARED_LIBS)
        # transient libraries
        target_link_libraries(${UTILNAME} PRIVATE ${OPENSSL_LIBRARIES} ${Libssh2_LIBRARY})
        if(WIN32)
            target_link_libraries(${UTILNAME} PRIVATE wsock32 ws2_32)
        endif()
    endif()
endforeach()
add_definitions("-DVERSION=\"${PROJECT_VERSION}\"")

install (TARGETS ${UTIL_TARGET}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)