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
option(ENABLE_EXAMPLES "Includes example plugins and programs [default: off]" OFF)

# libxzero platform feature flags
option(XZERO_ENABLE_INOTIFY "Build with inotify support [default: on]" ON)
option(XZERO_ENABLE_O_TMPFILE "Try to open temporary files with O_TMPFILE flag before falling back to standard behaviour [default: on]" ON)

# Xzero feature flags
option(ENABLE_ACCEPT4 "Builds with support for accept4() system-call, if found [default: on]" ON)
option(ENABLE_MULTI_ACCEPT "Builds listener acceptor with support for repeative accept() calls  [default: on]" ON)
option(ENABLE_PCRE "With PCRE support [default: off]" ON)
option(ENABLE_INOTIFY "Build with inotify support [default: on]" ON)
option(ENABLE_TCP_DEFER_ACCEPT "Builds with support for TCP_DEFER_ACCEPT, if found [default: on]" ON)

# libxzero implementation feature flags
option(XZERO_ENABLE_POST_FN_OPTIMIZATION "Optimize code path for post()'ing code blocks into current worker threads [default: OFF]" OFF)
option(XZERO_OPPORTUNISTIC_WRITE "Builds with support for opportunistic write() calls to client sockets [default: off]." OFF)
option(XZERO_WORKER_POST_LIBEV "Use libev's async method to post into HTTP workers instead of a locking queue [default: on]" ON)
option(XZERO_WORKER_RR "Uses round-robin instead of lead-load for request scheduling" ON)

### HTTP transport protocol support
# option(XZERO_ENABLE_HTTP1 "Build HTTP/1 protocol support [default: on]" ON)
# option(XZERO_ENABLE_HTTP2 "Build HTTP/2 protocol support [default: off]" OFF)
# option(XZERO_ENABLE_FCGI "Build FastCGI protocol support [default: off]" OFF)
# option(XZERO_ENABLE_WEBSOCKETS "Build WebSockets protocol support [default: off]" OFF)

set(CMAKE_CXX_FLAGS "-std=c++0x")                   # C++11
#set(CMAKE_CXX_FLAGS "-std=c++0x -stdlib=libc++")   # C++11 + libc++
#set(CMAKE_CXX_FLAGS "-std=c++1y -stdlib=libc++")   # C++14 + libc++
#set(CMAKE_CXX_FLAGS "-std=c++1z -stdlib=libc++")   # C++17 + libc++

CHECK_INCLUDE_FILES(sys/sendfile.h HAVE_SYS_SENDFILE_H)
CHECK_FUNCTION_EXISTS(sendfile HAVE_SENDFILE)
CHECK_FUNCTION_EXISTS(posix_fadvise HAVE_POSIX_FADVISE)
CHECK_FUNCTION_EXISTS(readahead HAVE_READAHEAD)

CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILES(sys/resource.h HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILES(sys/mman.h HAVE_SYS_MMAN_H)
CHECK_INCLUDE_FILES(sys/limits.h HAVE_SYS_LIMITS_H)
CHECK_INCLUDE_FILES(pwd.h HAVE_PWD_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_UUID_H)

CHECK_FUNCTION_EXISTS(sysconf HAVE_SYSCONF)
CHECK_FUNCTION_EXISTS(pathconf HAVE_PATHCONF)
CHECK_FUNCTION_EXISTS(accept4 HAVE_ACCEPT4)

CHECK_INCLUDE_FILES(pthread.h HAVE_PTHREAD_H)
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
add_subdirectory(test)
add_subdirectory(examples)
add_subdirectory(doc)
