cmake_minimum_required(VERSION 2.8.8)
project(libstx)

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src/stx
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})

# DEPENDENCIES
find_package(Threads)

set(STX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src/stx/protobuf)
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} ${STX_PROTOBUF_LIBS}")
set(MODULE_LDFLAGS "${MODULE_LDFLAGS} ${STX_PROTOBUF_LIBS}")

add_subdirectory(src/stx/mdb)
add_subdirectory(src/stx/http)
add_subdirectory(src/stx/json)
add_subdirectory(src/stx/rpc)

add_subdirectory(src/stx/net/mysql)
set(STX_MYSQL_LIBS ${STX_MYSQL_LIBS} PARENT_SCOPE)

add_subdirectory(src/stx)

set(STX_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)
set(STX_LDFLAGS ${MODULE_LDFLAGS} PARENT_SCOPE)
set(STX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src PARENT_SCOPE)
