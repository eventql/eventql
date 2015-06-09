include_directories(${CMAKE_CURRENT_BINARY_DIR})
file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(
    sensord-common OBJECT
    HostStatsSensor.cc
    SensorRepository.cc
    Sampler.cc
    ${PROTO_SRCS})

add_executable(
    sensord
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:sensord-common>
    sensord.cc)

target_link_libraries(
    sensord
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_MSG_LIBS})


