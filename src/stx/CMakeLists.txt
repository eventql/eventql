project(libstx)
cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Threads)
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
#CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)

CHECK_INCLUDE_FILES(pthread.h HAVE_PTHREAD_H)
CHECK_LIBRARY_EXISTS(pthread pthread_setname_np "" HAVE_PTHREAD_SETNAME_NP)
CHECK_LIBRARY_EXISTS(pthread pthread_setaffinity_np "" HAVE_PTHREAD_SETAFFINITY_NP)

CHECK_LIBRARY_EXISTS(stdc++ __cxa_pure_virtual "" COMPILER_RT_HAS_LIBSTDCXX)
CHECK_LIBRARY_EXISTS(m pow "" COMPILER_RT_HAS_LIBM)
CHECK_LIBRARY_EXISTS(dl dlopen "" COMPILER_RT_HAS_LIBDL)
CHECK_LIBRARY_EXISTS(pthread pthread_create "" COMPILER_RT_HAS_LIBPTHREAD)

if(NOT COMPILER_RT_HAS_LIBSTDCXX)
  message(FATAL_ERROR "libstdc++ not found")
endif()

include(cmake/FindCXX11HashMapAndSet.cmake)

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

option(ENABLE_PCRE "With PCRE support [default: on]" ON)
if(NOT HAVE_PCRE)
  if(ENABLE_PCRE)
    find_package(PCRE)
    if (PCRE_FOUND)
      set(HAVE_PCRE 1)
    endif()
  endif()
endif()

if (HAVE_PCRE)
  message(STATUS "PCRE found")
else()
  message(STATUS "PCRE not found")
endif()

# required for shm_open/shm_unlink
if(APPLE)
  set(RT_LIBRARIES)
else()
  set(RT_LIBRARIES rt)
endif()

if(OPENSSL_FOUND)
  message(STATUS "OpenSSL found")
else()
  message(STATUS "OpenSSL not found")
endif()

# PROTOBUF
find_program(PROTOBUF_PROTOC_EXECUTABLE NAMES protoc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(PROTOBUF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(PROTOBUF_INCLUDE_DIR ${PROTOBUF_INCLUDE_DIR} PARENT_SCOPE)
include(cmake/CompileProtobuf.cmake)

# INCLUDES
set(STX_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/stx
    ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${STX_INCLUDE})
set(STX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(STX_LDFLAGS "${STX_LDFLAGS} ${STX_PROTOBUF_LIBS}")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} ${STX_LDFLAGS}")
message(STATUS "libSTX ldflags: ${STX_LDFLAGS}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sysconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sysconfig.h)

# base
add_library(stx-base STATIC
    3rdparty/simdcomp/simdbitpacking.c
    3rdparty/simdcomp/simdintegratedbitpacking.c
    3rdparty/simdcomp/simdcomputil.c
    application.cc
    autoref.cc
    assets.cc
    buffer.cc
    bufferutil.cc
    cli/flagparser.cc
    cli/CLI.cc
    cli/CLICommand.cc
    cli/term.cc
    csv/CSVOutputStream.cc
    csv/BinaryCSVOutputStream.cc
    csv/CSVInputStream.cc
    csv/BinaryCSVInputStream.cc
    UnixTime.cc
    duration.cc
    exception.cc
    exceptionhandler.cc
    fnv.cc
    HMAC.cc
    human.cc
    ieee754.cc
    inspect.cc
    InternMap.cc
    io/file.cc
    io/FileLock.cc
    io/fileutil.cc
    io/filerepository.cc
    io/inputstream.cc
    io/outputstream.cc
    io/BufferedOutputStream.cc
    io/mmappedfile.cc
    io/pagemanager.cc
    ISO8601.cc
    CivilTime.cc
    Language.cc
    logging.cc
    logging/logoutputstream.cc
    net/dnscache.cc
    net/tcpserver.cc
    net/udpserver.cc
    net/udpsocket.cc
    net/inetaddr.cc
    net/tcpconnection.cc
    random.cc
    RegExp.cc
    rpc/RPCRequest.cc
    rpc/RPCContext.cc
    rpc/RPCStub.cc
    rpc/RPCService.cc
    SHA1.cc
    StackTrace.cc
    status.cc
    stats/statsdagent.cc
    stats/statsrepository.cc
    stats/statssink.cc
    stats/statsd.cc
    stringutil.cc
    thread/eventloop.cc
    thread/signalhandler.cc
    thread/threadpool.cc
    thread/FixedSizeThreadPool.cc
    thread/wakeup.cc
    uri.cc
    UTF8.cc
    util/Base64.cc
    util/binarymessagereader.cc
    util/binarymessagewriter.cc
    util/CumulativeHistogram.cc
    util/BitPackDecoder.cc
    util/BitPackEncoder.cc
    util/SimpleRateLimit.cc
    util/PersistentHashSet.cc
    VFS.cc
    wallclock.cc
    web/SecureCookie.cc)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
set(PROTOC_ARGS --proto_path ${CMAKE_CURRENT_SOURCE_DIR}/ PARENT_SCOPE)

add_library(stx-protobuf STATIC
    3rdparty/google/protobuf/compiler/parser.cc
    3rdparty/google/protobuf/descriptor.cc
    3rdparty/google/protobuf/descriptor.pb.cc
    3rdparty/google/protobuf/descriptor_database.cc
    3rdparty/google/protobuf/dynamic_message.cc
    3rdparty/google/protobuf/extension_set.cc
    3rdparty/google/protobuf/extension_set_heavy.cc
    3rdparty/google/protobuf/generated_message_reflection.cc
    3rdparty/google/protobuf/generated_message_util.cc
    3rdparty/google/protobuf/message.cc
    3rdparty/google/protobuf/message_lite.cc
    3rdparty/google/protobuf/reflection_ops.cc
    3rdparty/google/protobuf/repeated_field.cc
    3rdparty/google/protobuf/service.cc
    3rdparty/google/protobuf/text_format.cc
    3rdparty/google/protobuf/unknown_field_set.cc
    3rdparty/google/protobuf/wire_format.cc
    3rdparty/google/protobuf/wire_format_lite.cc
    3rdparty/google/protobuf/io/coded_stream.cc
    3rdparty/google/protobuf/io/gzip_stream.cc
    3rdparty/google/protobuf/io/printer.cc
    3rdparty/google/protobuf/io/tokenizer.cc
    3rdparty/google/protobuf/io/zero_copy_stream.cc
    3rdparty/google/protobuf/io/zero_copy_stream_impl.cc
    3rdparty/google/protobuf/io/zero_copy_stream_impl_lite.cc
    3rdparty/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
    3rdparty/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
    3rdparty/google/protobuf/stubs/common.cc
    3rdparty/google/protobuf/stubs/once.cc
    3rdparty/google/protobuf/stubs/stringprintf.cc
    3rdparty/google/protobuf/stubs/structurally_valid.cc
    3rdparty/google/protobuf/stubs/strutil.cc
    3rdparty/google/protobuf/stubs/substitute.cc
    Currency.cc # FIXME
    protobuf/MessageDecoder.cc
    protobuf/MessageEncoder.cc
    protobuf/MessagePrinter.cc
    protobuf/MessageObject.cc
    protobuf/MessageSchema.cc
    protobuf/DynamicMessage.cc
    protobuf/JSONEncoder.cc
    ${PROTO_SRCS})

add_executable(test-inputstream io/inputstream_test.cc)
target_link_libraries(test-inputstream stx-base)

add_executable(test-uri uri_test.cc)
target_link_libraries(test-uri stx-base)

add_executable(test-stringutil stringutil_test.cc)
target_link_libraries(test-stringutil stx-base)

add_executable(test-internmap InternMap_test.cc)
target_link_libraries(test-internmap stx-base)

add_executable(test-hmac HMAC_test.cc)
target_link_libraries(test-hmac stx-base)

add_executable(test-human human_test.cc)
target_link_libraries(test-human stx-base)

add_executable(test-secure-cookie web/SecureCookie_test.cc)
target_link_libraries(test-secure-cookie stx-base)

add_executable(test-sha1 SHA1_test.cc)
target_link_libraries(test-sha1 stx-base)

add_executable(test-protobuf protobuf/protobuf_test.cc)
target_link_libraries(test-protobuf stx-protobuf stx-json stx-base )

add_executable(test-persistenthashset util/PersistentHashSet_test.cc)
target_link_libraries(test-persistenthashset stx-base)

add_subdirectory(http)
add_subdirectory(json)
add_subdirectory(rpc)

set(STX_INCLUDE ${STX_INCLUDE} PARENT_SCOPE)
set(STX_LDFLAGS ${STX_LDFLAGS} PARENT_SCOPE)
set(STX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
