project(FNORD_FEEDS)

include_directories(../)
include_directories(../3rdparty)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(brokerd-client OBJECT
    BrokerClient.cc
    LocalFeed.cc
    RemoteFeed.cc
    RemoteFeedFactory.cc
    RemoteFeedReader.cc
    RemoteFeedWriter.cc
    FeedService.cc
    ${PROTO_SRCS})

add_executable(brokerd
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:brokerd-client>
    BrokerServlet.cc
    brokerd.cc)

add_executable(brokerctl
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:brokerd-client>
    brokerctl.cc)

