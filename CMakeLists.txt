cmake_minimum_required(VERSION 3.10)
project(ecs34proj4 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "" FORCE)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wpedantic -fPIC")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/StringUtils.cpp
    src/StringDataSink.cpp
    src/StringDataSource.cpp
    src/DSVWriter.cpp
    src/DSVReader.cpp
    src/XMLReader.cpp
    src/XMLWriter.cpp
    src/CSVBusSystem.cpp
    src/OpenStreetMap.cpp
)

add_library(ecs34-osm SHARED ${SOURCES})
target_link_libraries(ecs34-osm expat)

set_target_properties(ecs34-osm PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
enable_testing()

set(TEST_SOURCES
    testsrc/StringUtilsTest.cpp
    testsrc/StringDataSinkTest.cpp
    testsrc/StringDataSourceTest.cpp
    testsrc/DSVTest.cpp
    testsrc/XMLTest.cpp
    testsrc/CSVBusSystemTest.cpp
    testsrc/OpenStreetMapTest.cpp
)

set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME "${TEST_SOURCE}" NAME_WE)
    add_executable("${TEST_NAME}" "${TEST_SOURCE}" ${SOURCES})
    target_compile_definitions("${TEST_NAME}" PRIVATE "DATA_DIR=${DATA_DIR}")
    target_link_libraries("${TEST_NAME}" ecs34-osm GTest::gtest GTest::gtest_main Threads::Threads expat gcov)
    target_compile_options("${TEST_NAME}" PRIVATE --coverage)
    set_target_properties("${TEST_NAME}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/testbin"
    )
    add_test(NAME "${TEST_NAME}" COMMAND "${TEST_NAME}" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()

add_custom_target(coverage
    COMMAND lcov --capture --directory . --output-file ${CMAKE_CURRENT_SOURCE_DIR}/htmlcov/coverage.info --ignore-errors inconsistent,source,mismatch
    COMMAND lcov --remove ${CMAKE_CURRENT_SOURCE_DIR}/htmlcov/coverage.info '/usr/*' '*/testsrc/*' --output-file ${CMAKE_CURRENT_SOURCE_DIR}/htmlcov/coverage.info
    COMMAND genhtml ${CMAKE_CURRENT_SOURCE_DIR}/htmlcov/coverage.info --output-directory ${CMAKE_CURRENT_SOURCE_DIR}/htmlcov
    DEPENDS ${TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generate Coverage Report"
)

add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/obj
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/testobj
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/testbin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/htmlcov
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
    COMMENT "Clean All Build Target"
)
