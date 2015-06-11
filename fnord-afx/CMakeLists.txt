project(FNORD_AFX)

include_directories(../)
include_directories(../3rdparty)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(fnord-afx OBJECT
    ArtifactIndex.cc
    ArtifactIndexMergeStrategy.cc
    ArtifactIndexReplication.cc
    ArtifactReplication.cc
    ${PROTO_SRCS})
