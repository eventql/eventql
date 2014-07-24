set(libxzero_SRC
  HttpClient.cpp
  HttpConnection.cpp
  HttpFile.cpp
  HttpFileMgr.cpp
  HttpMessageParser.cpp
  HttpRequest.cpp
  HttpServer.cpp
  HttpStatus.cpp
  HttpWorker.cpp
  HttpVary.cpp
  http2.cpp
  hpack.cpp
)

add_library(xzero SHARED ${libxzero_SRC})
target_link_libraries(xzero ${BASE_LIBRARIES})
set_target_properties(xzero PROPERTIES VERSION ${LIBXZERO_VERSION})
set_target_properties(xzero PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -DBUILD_XZERO=1")
install(TARGETS xzero DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
