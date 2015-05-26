project(FNORD_AFX)

include_directories(../)
include_directories(../3rdparty)
file(GLOB PROTO_FILES *.proto)

foreach(PROTO_FILE ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
  set(PROTO_SRCS ${PROTO_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME}.pb.cc)
endforeach()

add_custom_target(fnord-afx-protos ALL
    COMMAND protoc --cpp_out=. -I${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${PROTO_FILES})

set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED true)

add_library(fnord-afx OBJECT
    ArtifactIndex.cc
    ArtifactIndexMergeStrategy.cc
    ArtifactIndexReplication.cc
    ArtifactReplication.cc
    ${PROTO_SRCS})

add_dependencies(fnord-afx fnord-afx-protos)
