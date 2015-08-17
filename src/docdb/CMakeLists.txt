file(GLOB PROTO_FILES *.proto */*.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(dxa-docdb OBJECT
    ${PROTO_SRCS}
    Document.cc
    DocumentDB.cc
    DocumentDBServlet.cc)

