project(FNORD_FEEDS)

include_directories(../)
include_directories(../3rdparty)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(fnord-feeds OBJECT
    BrokerClient.cc
    LocalFeed.cc
    RemoteFeed.cc
    RemoteFeedFactory.cc
    RemoteFeedReader.cc
    RemoteFeedWriter.cc
    FeedService.cc
    ${PROTO_SRCS})

add_executable(brokerd
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    BrokerServlet.cc
    brokerd.cc)

target_link_libraries(brokerd ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

add_executable(brokerctl
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    brokerctl.cc)

target_link_libraries(brokerctl ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

#add_executable(fnord-logstream-service-example
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:sstable>
#    feeds_example.cc)
#
#add_executable(tests/test-logstream-service
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:sstable>
#    feeds_test.cc)
#
