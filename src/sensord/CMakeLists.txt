file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(
    sensord-client OBJECT
    SensorPushServlet.cc
    SensorSampleFeed.cc
    ${PROTO_SRCS})

add_executable(
    sensord
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-http>
    HostStatsSensor.cc
    Sensor.cc
    SensorRepository.cc
    Sampler.cc
    ${PROTO_SRCS}
    sensord.cc)

target_link_libraries(
    sensord
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_LIBS})


