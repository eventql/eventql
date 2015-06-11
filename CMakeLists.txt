cmake_minimum_required(VERSION 2.8.8)
project(sensord)

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
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})
set(SENSORD_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)

# DEPENDENCIES
find_package(Threads)

if(NOT DEFINED NO_BUNDLED_LIBFNORD)
  include_directories(${PROJECT_BINARY_DIR}/3rdparty/libfnord/)
  include_directories(3rdparty/libfnord/)
  include_directories(3rdparty/libfnord/3rdparty/)
  add_subdirectory(3rdparty/libfnord/fnord-base)
  add_subdirectory(3rdparty/libfnord/fnord-msg)
  add_subdirectory(3rdparty/libfnord/fnord-http)
endif()

add_subdirectory(src/sensord)
