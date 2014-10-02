set(libxzero_SRC
  # base
  Base64.cpp
  Buffer.cpp
  DateTime.cpp
  IdleTimeout.cpp
  WallClock.cpp
  TimeSpan.cpp
  Tokenizer.cpp

  logging/LogAggregator.cpp
  logging/LogLevel.cpp
  logging/LogSource.cpp
  logging/LogTarget.cpp

  executor/DirectExecutor.cpp
  executor/DirectLoopExecutor.cpp
  executor/Scheduler.cpp
  executor/ThreadedExecutor.cpp
  executor/ThreadPool.cpp

  io/Selectable.cpp

  net/ByteArrayEndPoint.cpp
  net/Cidr.cpp
  net/Connection.cpp
  net/ConnectionFactory.cpp
  net/Connector.cpp
  net/EndPoint.cpp
  net/EndPointWriter.cpp
  net/InetConnector.cpp
  net/InetEndPoint.cpp
  net/LocalConnector.cpp
  net/Server.cpp

  # http common
  http/HeaderField.cpp
  http/HeaderFieldList.cpp
  http/HttpChannel.cpp
  http/HttpConnectionFactory.cpp
  http/HttpDateGenerator.cpp
  http/HttpInput.cpp
  http/HttpListener.cpp
  http/HttpOutput.cpp
  http/HttpOutputFilter.cpp
  http/HttpRequest.cpp
  http/HttpResponse.cpp
  http/HttpStatus.cpp
  http/HttpTransport.cpp
  http/HttpVersion.cpp

  # http support
  http/HttpFileHandler.cpp
  http/HttpService.cpp

  # http/mock
  http/mock/MockTransport.cpp

  # http/1
  http/v1/HttpConnection.cpp
  http/v1/Http1ConnectionFactory.cpp
  http/v1/HttpGenerator.cpp
  http/v1/HttpParser.cpp
  http/v1/Http1Input.cpp
  http/v1/Http1Channel.cpp

  # http/2
  #...

  # support
  support/LibevClock.cpp
  support/LibevScheduler.cpp
  support/LibevSelector.cpp
)

add_library(xzero SHARED ${libxzero_SRC})
target_link_libraries(xzero ev pthread ${ZLIB_LIBRARIES})
set_target_properties(xzero PROPERTIES VERSION ${LIBXZERO_VERSION})
set_target_properties(xzero PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_XZERO=1")
#set_target_properties(xzero PROPERTIES LINK_FLAGS "${BASE_LDFLAGS}")
install(TARGETS xzero DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
