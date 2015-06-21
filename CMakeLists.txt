cmake_minimum_required(VERSION 2.8.8)
project(libfnord)

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src/fnord # protobuf name mangling via relative file paths sucks :(
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})

# DEPENDENCIES
find_package(Threads)

set(FNORD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src/fnord/protobuf)
add_subdirectory(src/fnord/mdb)
add_subdirectory(src/fnord/http)
add_subdirectory(src/fnord/json)
add_subdirectory(src/fnord/rpc)
add_subdirectory(src/fnord/charts)
add_subdirectory(src/fnord)

set(FNORD_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)
set(FNORD_LIBS ${FNORD_PROTOBUF_LIBS} ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)
set(FNORD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src PARENT_SCOPE)
