cmake_minimum_required(VERSION 2.8)

project(cortex-base CXX)
set(CORTEX_BASE_VERSION "0.11.0-dev")

include(CortexCommon)
include(FindPkgConfig)
include(FindDoxygen)
include(FindOpenSSL)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckVariableExists)
include(CheckTypeSize)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(CMakeDetermineCCompiler)

CHECK_INCLUDE_FILES(sys/sendfile.h HAVE_SYS_SENDFILE_H)
CHECK_FUNCTION_EXISTS(sendfile HAVE_SENDFILE)
CHECK_FUNCTION_EXISTS(posix_fadvise HAVE_POSIX_FADVISE)
CHECK_FUNCTION_EXISTS(readahead HAVE_READAHEAD)
CHECK_FUNCTION_EXISTS(pread HAVE_PREAD)

CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILES(sys/resource.h HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILES(sys/mman.h HAVE_SYS_MMAN_H)
CHECK_INCLUDE_FILES(sys/limits.h HAVE_SYS_LIMITS_H)
CHECK_INCLUDE_FILES(pwd.h HAVE_PWD_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
CHECK_INCLUDE_FILES(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILES(execinfo.h HAVE_EXECINFO_H)
CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_UUID_H)

CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS(daemon HAVE_DAEMON)
CHECK_FUNCTION_EXISTS(sysconf HAVE_SYSCONF)
CHECK_FUNCTION_EXISTS(pathconf HAVE_PATHCONF)
CHECK_FUNCTION_EXISTS(accept4 HAVE_ACCEPT4)
CHECK_FUNCTION_EXISTS(pipe2 HAVE_PIPE2)
CHECK_FUNCTION_EXISTS(dup2 HAVE_DUP2)
CHECK_FUNCTION_EXISTS(dladdr HAVE_DLADDR)
CHECK_FUNCTION_EXISTS(fork HAVE_FORK)
CHECK_FUNCTION_EXISTS(backtrace HAVE_BACKTRACE)
CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)

CHECK_INCLUDE_FILES(pthread.h HAVE_PTHREAD_H)
CHECK_LIBRARY_EXISTS(pthread pthread_setname_np "" HAVE_PTHREAD_SETNAME_NP)
CHECK_LIBRARY_EXISTS(pthread pthread_setaffinity_np "" HAVE_PTHREAD_SETAFFINITY_NP)

CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
if(HAVE_SYS_INOTIFY_H)
  CHECK_FUNCTION_EXISTS(inotify_init1 HAVE_INOTIFY_INIT1)
endif(HAVE_SYS_INOTIFY_H)

CHECK_INCLUDE_FILES(zlib.h HAVE_ZLIB_H)
if(HAVE_ZLIB_H)
  CHECK_LIBRARY_EXISTS(z deflate "" HAVE_LIBZ)
  set(ZLIB_LIBRARIES z)
endif(HAVE_ZLIB_H)

CHECK_INCLUDE_FILES(bzlib.h HAVE_BZLIB_H)
if(HAVE_BZLIB_H)
  CHECK_LIBRARY_EXISTS(bz2 BZ2_bzCompressInit "" HAVE_LIBBZ2)
  set(BZIP2_LIBRARIES bz2)
endif(HAVE_BZLIB_H)

# PCRE
option(ENABLE_PCRE "With PCRE support [default: on]" ON)
if(ENABLE_PCRE)
  pkg_check_modules(PCRE REQUIRED libpcre>=7.0)
else()
  pkg_check_modules(PCRE libpcre>=7.0)
endif()

# required for shm_open/shm_unlink
if(APPLE)
  set(RT_LIBRARIES)
else()
  set(RT_LIBRARIES rt)
endif()

if(NOT OPENSSL_FOUND)
  message(FATAL_ERROR "OpenSSL not found ;-(")
else()
  message(STATUS "OpenSSL found ;-)")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sysconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sysconfig.h)

set(cortex_base_SRC
  Application.cc
  Base64.cc
  Buffer.cc
  DateTime.cc
  IEEE754.cc
  IdleTimeout.cc
  MimeTypes.cc
  Random.cc
  RegExp.cc
  RuntimeError.cc
  StackTrace.cc
  Status.cc
  StringUtil.cc
  TimeSpan.cc
  Tokenizer.cc
  Uri.cc
  WallClock.cc
  inspect.cc

  cli/CLI.cc
  cli/Flags.cc

  executor/SafeCall.cc
  executor/Executor.cc
  executor/DirectExecutor.cc
  executor/PosixScheduler.cc
  executor/Scheduler.cc
  executor/ThreadedExecutor.cc
  executor/ThreadPool.cc

  hash/FNV.cc

  io/GzipFilter.cc
  io/File.cc
  io/FileDescriptor.cc
  io/FileRef.cc
  io/FileRepository.cc
  io/Filter.cc
  io/FileUtil.cc
  io/LocalFile.cc
  io/LocalFileRepository.cc
  io/MemoryFile.cc
  io/MemoryFileRepository.cc
  io/MemoryMap.cc
  io/PageManager.cc

  logging/LogAggregator.cc
  logging/LogLevel.cc
  logging/LogSource.cc
  logging/LogTarget.cc
  logging.cc

  net/ByteArrayEndPoint.cc
  net/Cidr.cc
  net/Connection.cc
  net/ConnectionFactory.cc
  net/Connector.cc
  net/DatagramConnector.cc
  net/DatagramEndPoint.cc
  net/DnsClient.cc
  net/EndPoint.cc
  net/EndPointWriter.cc
  net/InetConnector.cc
  net/InetEndPoint.cc
  net/LocalConnector.cc
  net/LocalDatagramConnector.cc
  net/LocalDatagramEndPoint.cc
  net/Server.cc
  net/SslConnector.cc
  net/SslContext.cc
  net/SslEndPoint.cc
  net/UdpClient.cc
  net/UdpConnector.cc
  net/UdpEndPoint.cc

  thread/SignalHandler.cc
  thread/Wakeup.cc

  util/BinaryMessageReader.cc
  util/BinaryMessageWriter.cc
  util/SimpleRateLimit.cc
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CORTEX_BASE_LIBRARIES pthread dl
    ${ZLIB_LIBRARIES} ${OPENSSL_LIBRARIES} ${PCRE_LIBRARIES} ${RT_LIBRARIES})

# libcortex-base.a
add_library(cortex-base STATIC ${cortex_base_SRC})
target_link_libraries(cortex-base ${CORTEX_BASE_LIBRARIES})
#set_target_properties(cortex-base PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_CORTEX_BASE=1")
#install(TARGETS cortex-base DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING PATTERN "*.h"
                       PATTERN "CMakeFiles" EXCLUDE)

# test-base
file(GLOB_RECURSE cortex_base_test_SRC "*-test.cc")
add_executable(test-base ${cortex_base_test_SRC})
target_link_libraries(test-base cortex-base gtest gtest_main)

# pkg-config target
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cortex-base.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/cortex-base.pc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cortex-base.pc 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
