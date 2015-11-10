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
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:brokerd-client>
    BrokerServlet.cc
    brokerd.cc)

target_link_libraries(brokerd stx-http stx-protobuf stx-json sstable stx-base )

add_executable(brokerctl
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:brokerd-client>
    brokerctl.cc)

target_link_libraries(brokerctl stx-http stx-protobuf stx-json sstable stx-base)
