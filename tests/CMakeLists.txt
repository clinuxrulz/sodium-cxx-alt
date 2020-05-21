cmake_minimum_required( VERSION 2.8 )

project(sodium_tests)

set( SODIUM_TESTS_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( Boost REQUIRED )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
find_package( CPPUNIT REQUIRED )

if ( MSVC )
    add_compile_options(-bigobj)
endif ( MSVC )

file( GLOB 
    SODIUM_TESTS_INCLUDE_FILES 
    ${SODIUM_TESTS_BASE_DIR}/*.h
    )

file( GLOB_RECURSE 
    SODIUM_TESTS_SOURCE_FILES
    ${SODIUM_TESTS_BASE_DIR}/*.cpp
    )

include_directories( ${Boost_INCLUDE_DIR} ${CPPUNIT_INCLUDE_DIR} ${SODIUM_TESTS_BASE_DIR}/.. )

set( ALL_SOURCES ${SODIUM_TESTS_INCLUDE_FILES} ${SODIUM_TESTS_SOURCE_FILES} )

add_executable( sodium_tests ${ALL_SOURCES} )

target_link_libraries( sodium_tests ${CPPUNIT_LIBRARY} )
