cmake_minimum_required( VERSION 2.8 )

project(sodium_tests)

set( SODIUM_TESTS_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( Boost REQUIRED )

file( GLOB 
    SODIUM_TESTS_INCLUDE_FILES 
    ${SODIUM_TESTS_BASE_DIR}/*.h
    )

file( GLOB_RECURSE 
    SODIUM_TESTS_SOURCE_FILES
    ${SODIUM_TESTS_BASE_DIR}/*.cpp
    )

include_directories( ${Boost_INCLUDE_DIR} ${SODIUM_TESTS_BASE_DIR}/.. )

set( ALL_SOURCES ${SODIUM_TESTS_INCLUDE_FILES} ${SODIUM_TESTS_SOURCE_FILES} )

add_executable( sodium_tests ${ALL_SOURCES} )
