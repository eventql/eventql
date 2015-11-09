add_executable(dns-resolver dns-resolver.cc)
target_link_libraries(dns-resolver cortex-base)

add_executable(logging-simple logging-simple.cc)
target_link_libraries(logging-simple cortex-base)

add_executable(compress compress.cc)
target_link_libraries(compress cortex-base)

add_executable(echo-server echo-server.cc)
target_link_libraries(echo-server cortex-base)

add_executable(echo-server2 echo-server2.cc)
target_link_libraries(echo-server2 cortex-base)

add_executable(threaded-executor threaded-executor.cc)
target_link_libraries(threaded-executor cortex-base)

add_executable(udp-echo-server udp-echo-server.cc)
target_link_libraries(udp-echo-server cortex-base)

add_executable(udp-echo-client udp-echo-client.cc)
target_link_libraries(udp-echo-client cortex-base)
