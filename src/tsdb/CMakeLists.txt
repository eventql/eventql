# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
project(FNORD_TSDB)

include_directories(../)
include_directories(../3rdparty)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(fnord-tsdb OBJECT
    CompactionWorker.cc
    ReplicationWorker.cc
    TSDBClient.cc
    TSDBNode.cc
    TSDBServlet.cc
    RecordSet.cc
    StreamChunk.cc
    CSTableIndex.cc
    TimeWindowPartitioner.cc
    ${PROTO_SRCS})

add_executable(tests/test-recordset
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:dproc>
    RecordSet_test.cc)

target_link_libraries(tests/test-recordset ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

add_executable(tests/test-tsdbnode
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:dproc>
    TSDBNode_test.cc)

target_link_libraries(tests/test-tsdbnode ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

add_executable(tests/test-tsdb-timewindowpartitioner
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:dproc>
    TimeWindowPartitioner_test.cc)

target_link_libraries(tests/test-tsdb-timewindowpartitioner ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})
