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
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} ${FNORD_PROTOBUF_LIBS}")
set(MODULE_LDFLAGS "${MODULE_LDFLAGS} ${FNORD_PROTOBUF_LIBS}")

add_subdirectory(src/fnord/mdb)
add_subdirectory(src/fnord/http)
add_subdirectory(src/fnord/json)
add_subdirectory(src/fnord/rpc)
add_subdirectory(src/fnord/charts)

add_subdirectory(src/fnord/net/mysql)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -l${FNORD_MYSQL_LIBS}")

add_subdirectory(src/fnord)

set(FNORD_MYSQL_LIBS ${FNORD_MYSQL_LIBS} PARENT_SCOPE)

set(FNORD_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)
set(FNORD_LDFLAGS ${MODULE_LDFLAGS} PARENT_SCOPE)
set(FNORD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src PARENT_SCOPE)
