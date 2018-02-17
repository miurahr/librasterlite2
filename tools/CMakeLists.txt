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
        Threads::Threads)
endforeach()
install (TARGETS ${UTIL_TARGET}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)