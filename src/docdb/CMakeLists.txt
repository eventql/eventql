file(GLOB PROTO_FILES *.proto */*.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_executable(dxa-docdb
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:auth>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:libcstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:chartsql-lib>
    $<TARGET_OBJECTS:fnord-chart>
    ${PROTO_SRCS}
    Document.cc
    DocumentDB.cc
    DocumentDBServlet.cc
    dxa-docdb.cc)

target_link_libraries(dxa-docdb
    ${CMAKE_THREAD_LIBS_INIT} ${STX_LIBS})

