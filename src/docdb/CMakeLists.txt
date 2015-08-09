file(GLOB PROTO_FILES *.proto */*.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_executable(dxa-docdb
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:auth>
    $<TARGET_OBJECTS:tsdb-server>
    ${PROTO_SRCS}
    Document.cc
    DocumentDB.cc
    DocumentDBServlet.cc
    dxa-docdb.cc)

target_link_libraries(dxa-docdb stx-http stx-base stx-protobuf stx-json sstable cstable chartsql cplot)
