project(FNORD_FEEDS)

include_directories(../)
include_directories(../3rdparty)
file(GLOB PROTO_FILES *.proto)

foreach(PROTO_FILE ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
  set(PROTO_SRCS ${PROTO_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME}.pb.cc)
endforeach()

add_custom_target(fnord-feeds-protos ALL
    COMMAND protoc --cpp_out=. -I${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${PROTO_FILES})

set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED true)

add_library(fnord-feeds OBJECT
    BrokerClient.cc
    LocalFeed.cc
    RemoteFeed.cc
    RemoteFeedFactory.cc
    RemoteFeedReader.cc
    RemoteFeedWriter.cc
    FeedService.cc
    ${PROTO_SRCS})

add_dependencies(fnord-feeds fnord-feeds-protos)

add_executable(brokerd
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
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
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    brokerctl.cc)

target_link_libraries(brokerctl ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

#add_executable(fnord-logstream-service-example
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-sstable>
#    feeds_example.cc)
#
#add_executable(tests/test-logstream-service
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-sstable>
#    feeds_test.cc)
#
