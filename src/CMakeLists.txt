cmake_minimum_required(VERSION 2.8.8)
project(libstx)

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/stx
    ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${MODULE_INCLUDE})

# DEPENDENCIES
find_package(Threads)

set(STX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(stx/protobuf)
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} ${STX_PROTOBUF_LIBS}")
set(MODULE_LDFLAGS "${MODULE_LDFLAGS} ${STX_PROTOBUF_LIBS}")

add_subdirectory(stx/mdb)
add_subdirectory(stx/http)
add_subdirectory(stx/json)
add_subdirectory(stx/rpc)

add_subdirectory(stx/net/mysql)
set(STX_MYSQL_LIBS ${STX_MYSQL_LIBS} PARENT_SCOPE)

add_subdirectory(stx)

set(STX_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)
set(STX_LDFLAGS ${MODULE_LDFLAGS} PARENT_SCOPE)
set(STX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
