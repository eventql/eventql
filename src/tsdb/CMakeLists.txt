# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(tsdb-client OBJECT
    TSDBClient.cc
    TimeWindowPartitioner.cc
    CSTableIndex.cc
    ${PROTO_SRCS})

add_library(tsdb-server OBJECT
    TSDBClient.cc
    TimeWindowPartitioner.cc
    CSTableIndex.cc
    SQLEngine.cc
    TSDBNode.cc
    TSDBServlet.cc
    TSDBTableProvider.cc
    TSDBTableRef.cc
    RecordSet.cc
    Partition.cc
    CompactionWorker.cc
    ReplicationWorker.cc
    ${PROTO_SRCS})

add_executable(tests/test-recordset
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    RecordSet_test.cc)

target_link_libraries(tests/test-recordset ${FNORD_LIBS})

add_executable(tests/test-tsdbnode
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    TSDBNode_test.cc)

target_link_libraries(tests/test-tsdbnode ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

add_executable(tests/test-tsdb-timewindowpartitioner
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    TimeWindowPartitioner_test.cc)

target_link_libraries(tests/test-tsdb-timewindowpartitioner ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

add_executable(tsdbd
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    tsdbd.cc)

target_link_libraries(tsdbd ${FNORD_LIBS})


