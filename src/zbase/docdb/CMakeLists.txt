file(GLOB PROTO_FILES *.proto */*.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(dxa-docdb OBJECT
    ${PROTO_SRCS}
    Document.cc
    DocumentDB.cc
    DocumentDBServlet.cc)

add_dependencies(dxa-docdb customerdirectory)

file(GLOB INVPROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(INVPROTO_SRCS INVPROTO_HDRS ${INVPROTO_FILES})

add_library(inventoryd-client OBJECT
    DocIndex.cc
    ItemRef.cc
    Document.cc
    ${INVPROTO_SRCS})

add_dependencies(inventoryd-client customerdirectory)
