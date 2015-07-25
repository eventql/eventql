cmake_minimum_required(VERSION 2.8)

project(cortex-http CXX)
set(CORTEX_HTTP_VERSION "0.11.0-dev")

include(CortexCommon)

set(cortex_http_SRC
  # common
  BadMessage.cc
  Cookies.cc
  HeaderField.cc
  HeaderFieldList.cc
  HttpBufferedInput.cc
  HttpChannel.cc
  HttpConnectionFactory.cc
  HttpDateGenerator.cc
  HttpFileHandler.cc
  HttpInput.cc
  HttpListener.cc
  HttpMethod.cc
  HttpOutput.cc
  HttpOutputCompressor.cc
  HttpRequest.cc
  HttpResponse.cc
  HttpService.cc
  HttpStatus.cc
  HttpTransport.cc
  HttpVersion.cc

  # transport: mock
  mock/Transport.cc
  mock/Input.cc

  # transport: fastcgi
  fastcgi/bits.cc
  fastcgi/Connection.cc
  fastcgi/ConnectionFactory.cc
  fastcgi/Generator.cc
  fastcgi/RequestParser.cc
  fastcgi/ResponseParser.cc

  # transport: http/1.1
  http1/Channel.cc
  http1/ConnectionFactory.cc
  http1/Connection.cc
  http1/Generator.cc
  http1/Parser.cc
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sysconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sysconfig.h)

set(CORTEX_HTTP_LIBRARIES cortex-base)

# libcortex-http.a
add_library(cortex-http STATIC ${cortex_http_SRC})
#set_target_properties(cortex-http PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_CORTEX_HTTP=1")
target_link_libraries(cortex-http ${CORTEX_HTTP_LIBRARIES})
install(TARGETS cortex-http DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING PATTERN "*.h"
                       PATTERN "CMakeFiles" EXCLUDE)

# test-http
file(GLOB_RECURSE cortex_http_test_SRC "*-test.cc")
add_executable(test-http ${cortex_http_test_SRC})
target_link_libraries(test-http cortex-http cortex-base gtest gtest_main)

# pkg-config target
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cortex-http.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/cortex-http.pc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cortex-http.pc 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
