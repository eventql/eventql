add_executable(http-hello1 http-hello1.cc)
target_link_libraries(http-hello1 cortex-http)

add_executable(http-exceptions http-exceptions.cc)
target_link_libraries(http-exceptions cortex-http)

add_executable(http-post-body http-post-body.cc)
target_link_libraries(http-post-body cortex-http)

add_executable(http-service http-service.cc)
target_link_libraries(http-service cortex-http)

add_executable(http-staticfile http-staticfile.cc)
target_link_libraries(http-staticfile cortex-http)

add_executable(http-threaded-nbnio http-threaded-nbnio.cc)
target_link_libraries(http-threaded-nbnio cortex-http)
