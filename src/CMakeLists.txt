# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
file(GLOB PROTO_FILES tsdb/*.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(tsdb-client OBJECT
    tsdb/TSDBClient.cc
    tsdb/TimeWindowPartitioner.cc
    tsdb/FixedShardPartitioner.cc
    ${PROTO_SRCS})

add_library(tsdb-server OBJECT
    tsdb/TSDBClient.cc
    tsdb/TimeWindowPartitioner.cc
    tsdb/FixedShardPartitioner.cc
    tsdb/CSTableIndex.cc
    tsdb/SQLEngine.cc
    tsdb/TSDBService.cc
    tsdb/TSDBServlet.cc
    tsdb/TSDBTableProvider.cc
    tsdb/TSDBTableRef.cc
    tsdb/RecordSet.cc
    tsdb/Partition.cc
    tsdb/PartitionMap.cc
    tsdb/PartitionReader.cc
    tsdb/PartitionWriter.cc
    tsdb/PartitionSnapshot.cc
    tsdb/PartitionReplication.cc
    tsdb/LogPartitionWriter.cc
    tsdb/LogPartitionReplication.cc
    tsdb/LazyPartition.cc
    tsdb/Table.cc
    tsdb/ReplicationWorker.cc
    tsdb/ReplicationScheme.cc
    tsdb/ReplicationState.cc
    tsdb/StaticPartitionWriter.cc
    ${PROTO_SRCS})

add_executable(test-recordset
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:libcstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    tsdb/RecordSet_test.cc)

target_link_libraries(test-recordset ${FNORD_LIBS})

add_executable(test-tsdb-timewindowpartitioner
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:libcstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    tsdb/TimeWindowPartitioner_test.cc)

target_link_libraries(test-tsdb-timewindowpartitioner ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

add_executable(tsdbd
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:libcstable>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    tsdb/tsdbd.cc)

target_link_libraries(tsdbd ${FNORD_LIBS})


