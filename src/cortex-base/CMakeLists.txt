cmake_minimum_required(VERSION 2.8)

project(cortex-base CXX)
set(CORTEX_BASE_VERSION "0.11.0-dev")

include(CortexCommon)
include(FindPkgConfig)
include(FindDoxygen)
include(FindOpenSSL)

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
