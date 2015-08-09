#file(GLOB PROTO_FILES *.proto */*.proto)
#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_executable(dxa-master
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:tsdb-server>
    ConfigDirectoryMaster.cc
    MasterServlet.cc
    dxa-master.cc)

target_link_libraries(dxa-master stx-http stx-base stx-protobuf stx-json stx-json sstable cstable chartsql cplot)
