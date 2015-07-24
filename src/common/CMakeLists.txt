file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(customerdirectory OBJECT
    CustomerDirectory.cc
    ${PROTO_SRCS})

