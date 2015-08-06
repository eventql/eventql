#file(GLOB PROTO_FILES *.proto */*.proto)
#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_executable(dxa-master
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:libcstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:chartsql-lib>
    $<TARGET_OBJECTS:fnord-chart>
#    ${PROTO_SRCS}
    ConfigDirectoryMaster.cc
    MasterServlet.cc
    dxa-master.cc)

target_link_libraries(dxa-master
    ${CMAKE_THREAD_LIBS_INIT} ${STX_LIBS})

