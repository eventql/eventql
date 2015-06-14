cmake_minimum_required(VERSION 2.8.8)
project(libfnord)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

# CFLAGS
set(CMAKE_CXX_FLAGS "-ftemplate-depth=500 ${CMAKE_CXX_FLAGS}")

if(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -stdlib=libc++ ")
else()
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "-ldl -static-libstdc++ -static-libgcc")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
  set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
  set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
endif()

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src/fnord/protobuf
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})

# DEPENDENCIES
find_package(Threads)

set(FNORD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src/fnord)
add_subdirectory(src/fnord/protobuf)
add_subdirectory(src/fnord/mdb)
add_subdirectory(src/fnord/http)
add_subdirectory(src/fnord/json)
add_subdirectory(src/fnord/rpc)
add_subdirectory(src/fnord/charts)

set(FNORD_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)
set(FNORD_LIBS ${FNORD_PROTOBUF_LIBS} ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)
set(FNORD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src PARENT_SCOPE)
