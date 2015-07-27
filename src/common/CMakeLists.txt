file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(customerdirectory OBJECT
    CustomerConfig.cc
    CustomerDirectory.cc
    SessionSchema.cc
    TableDefinition.cc
    ${PROTO_SRCS})

