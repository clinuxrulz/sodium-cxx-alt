cmake_minimum_required( VERSION 2.8 )

set( CMAKE_INSTALL_PREFIX /usr/local )

project(sodium)

set( SODIUM_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sodium )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( Boost REQUIRED )

file( GLOB 
    SODIUM_INCLUDE_FILES 
    ${SODIUM_BASE_DIR}/*.h
    )

file( GLOB 
    SODIUM_IMPL_INCLUDE_FILES
    ${SODIUM_BASE_DIR}/impl/*.h
    )

file( GLOB_RECURSE 
    SODIUM_SOURCE_FILES
    ${SODIUM_BASE_DIR}/*.cpp
    )

file(
    COPY ${SODIUM_INCLUDE_FILES} 
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/sodium
    )

file(
    COPY ${SODIUM_IMPL_INCLUDE_FILES} 
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/sodium/impl
    )

include_directories( ${Boost_INCLUDE_DIR} )

set( ALL_SOURCES ${SODIUM_INCLUDE_FILES} ${SODIUM_SOURCE_FILES} )

add_library( sodium SHARED ${ALL_SOURCES} )
add_library( libsodium STATIC ${ALL_SOURCES} )

set_target_properties( libsodium PROPERTIES OUTPUT_NAME sodium )

# INSTALL
# -------

install (
    TARGETS sodium libsodium
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Install header files
install (
    FILES ${SODIUM_INCLUDE_FILES}
    DESTINATION include/sodium
    )
