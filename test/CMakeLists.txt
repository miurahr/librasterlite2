
set(TEST_COMMANDS)
file(GLOB TESTCASE test*.c)
foreach(TESTF IN LISTS TESTCASE)
    get_filename_component(TESTNAME ${TESTF} NAME_WE)
    add_executable(${TESTNAME} ${TESTF})
    target_include_directories(${TESTNAME} PRIVATE ${HEADER_DIRS}
        ${LIBXML2_INCLUDE_DIR}
        ${SQLITE3_INCLUDE_DIRS}
        ${SPATIALITE_INCLUDE_DIRS})
    target_link_libraries(${TESTNAME} PRIVATE  $<TARGET_NAME:${TARGET_NAME}>
        ${LIBXML2_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        ${SPATIALITE_LIBRARIES})
    add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
    list(APPEND TEST_COMMANDS ${TESTNAME})
endforeach()

add_definitions(-DHAVE_CONFIG_H)

# copy test data to build dir
file(GLOB TEST_DATA *.gif *.jp2 *.jpg *.png *.sqlite *.svg *.tif *.ttf *.webp *.xml)
file(COPY ${TEST_DATA} map_samples sql_stmt_security_tests sql_stmt_tests
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ${TEST_COMMANDS})