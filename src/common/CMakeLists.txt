file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(customerdirectory OBJECT
    CustomerConfig.cc
    ConfigDirectory.cc
    SessionSchema.cc
    TableDefinition.cc
    ${PROTO_SRCS})

add_dependencies(customerdirectory tsdb-server)

add_library(auth OBJECT
    AnalyticsAuth.cc)

add_dependencies(auth customerdirectory)
