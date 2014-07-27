cmake_minimum_required(VERSION 2.8)

project(libxzero)
set(LIBXZERO_NAME "libxzero")
set(LIBXZERO_VERSION "0.11.0-dev")
set(LIBXZERO_HOMEPAGE_URL "http://xzero.io/")
set(LIBXZERO_BUGTRACKER_URL "https://github.com/xzero/libxzero/issues/")

include(FindPkgConfig)
include(FindDoxygen)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckVariableExists)
include(CheckTypeSize)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(CMakeDetermineCCompiler)

option(ENABLE_TESTS "Build unit tests [default: off]" OFF)
option(XZERO_ENABLE_EXAMPLES "Includes example plugins and programs [default: on]" ON)

# libxzero feature flags
option(XZERO_ENABLE_O_TMPFILE "Try to open temporary files with O_TMPFILE flag before falling back to standard behaviour [default: on]" ON)
option(XZERO_ENABLE_POST_FN_OPTIMIZATION "Optimize code path for post()'ing code blocks into current worker threads [default: OFF]" OFF)
option(XZERO_WORKER_POST_LIBEV "Use libev's async method to post into HTTP workers instead of a locking queue [default: on]" ON)
option(XZERO_WORKER_RR "Uses round-robin instead of lead-load for request scheduling" ON)
option(XZERO_OPPORTUNISTIC_WRITE "Builds with support for opportunistic write() calls to client sockets [default: off]." OFF)

# XXX do not use -std=c++11 here, as it's not supported on GCC 4.6 (default on Ubuntu 12.04)
set(CMAKE_CXX_FLAGS "-std=c++0x")

CHECK_INCLUDE_FILES(sys/sendfile.h HAVE_SYS_SENDFILE_H)
CHECK_FUNCTION_EXISTS(sendfile HAVE_SENDFILE)
CHECK_FUNCTION_EXISTS(posix_fadvise HAVE_POSIX_FADVISE)
CHECK_FUNCTION_EXISTS(readahead HAVE_READAHEAD)

CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILES(sys/resource.h HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILES(sys/mman.h HAVE_SYS_MMAN_H)
CHECK_INCLUDE_FILES(sys/limits.h HAVE_SYS_LIMITS_H)
CHECK_INCLUDE_FILES(pwd.h HAVE_PWD_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_UUID_H)

CHECK_FUNCTION_EXISTS(pathconf HAVE_PATHCONF)
CHECK_FUNCTION_EXISTS(accept4 HAVE_ACCEPT4)
CHECK_LIBRARY_EXISTS(pthread pthread_setname_np "" HAVE_PTHREAD_SETNAME_NP)
CHECK_LIBRARY_EXISTS(pthread pthread_setaffinity_np "" HAVE_PTHREAD_SETAFFINITY_NP)

if(ENABLE_INOTIFY)
  CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
  if(HAVE_SYS_INOTIFY_H)
    CHECK_FUNCTION_EXISTS(inotify_init1 HAVE_INOTIFY_INIT1)
  endif(HAVE_SYS_INOTIFY_H)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# libbase integration
if(EXISTS ${X0D_TOPLEVEL_3RDPARTY_DIR}/libbase)
  set(BASE_LIBRARIES base)
  message("libxzero: use libbase from ${X0D_TOPLEVEL_3RDPARTY_DIR}/libbase")
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libbase)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libbase)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libbase/include)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libbase/include)
  set(BASE_LIBRARIES base)
  message("libbase: use from 3rdparty/libbase")
else()
  pkg_check_modules(BASE REQUIRED "libbase")
  add_definitions(${BASE_CFLAGS})
  add_definitions(${BASE_CPPFLAGS})
  include_directories(${BASE_INCLUDE_DIR})
  include_directories(${BASE_INCLUDE_DIRS})
  link_directories(${BASE_LIBRARY_DIRS})

  message("libbase include dirs: ${BASE_INCLUDE_DIRS}")
  message("libbase cflags: ${BASE_CFLAGS}")
  message("libbase ldflags: ${BASE_LDFLAGS}")
  message("libbase libs: ${BASE_LIBRARIES}")
  message("libbase libdir: ${BASE_LIBRARY_DIRS}")
endif()

if(ENABLE_TESTS)
  CHECK_INCLUDE_FILES(gtest/gtest.h HAVE_GTEST_GTEST_H)
endif()

add_definitions(-DHAVE_SYSCONFIG_H)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/xzero/sysconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/xzero/sysconfig.h)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libxzero.pc.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/libxzero.pc)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/libxzero.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(docs)
