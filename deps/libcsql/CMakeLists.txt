cmake_minimum_required(VERSION 2.8.8)
project(chartsql)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src/csql
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})
set(CSQL_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)

# DEPENDENCIES
find_package(Threads)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_FLAGS "-std=c++0x -fvisibility=hidden -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor ${CMAKE_CXX_FLAGS} -Wno-predefined-identifier-outside-function")
set(CMAKE_C_FLAGS "-std=c11 -mno-omit-leaf-frame-pointer -fvisibility=hidden -fno-omit-frame-pointer -Wall -pedantic ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -lpthread")
set(CMAKE_C_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS_DEBUG}")

# OSX FLAGS
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ ")

# LINUX FLAGS
else()
  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -static-libstdc++ -static-libgcc")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
  set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
  set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
endif()

if(NOT HAVE_STX)
  add_subdirectory(deps/libstx)
  include_directories(${STX_INCLUDE})
  SET(HAVE_STX true)
endif()

if(NOT HAVE_CPLOT)
  add_subdirectory(deps/libcplot)
  include_directories(${CPLOT_INCLUDE})
  SET(HAVE_CPLOT true)
endif()

if(NOT HAVE_CSTABLE)
  add_subdirectory(deps/libcstable)
  include_directories(${CSTABLE_INCLUDE})
  SET(HAVE_CSTABLE true)
endif()

add_subdirectory(src/csql)
