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

add_library(fnord-tsdb OBJECT
    CompactionWorker.cc
    CSTableIndex.cc
    DerivedDataset.cc
    ReplicationWorker.cc
    IndexWorker.cc
    TSDBNode.cc
    TSDBServlet.cc
    RecordSet.cc
    StreamChunk.cc)

add_executable(tests/test-recordset
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    RecordSet_test.cc)

target_link_libraries(tests/test-recordset ${CMAKE_THREAD_LIBS_INIT})

add_executable(tests/test-tsdbnode
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    TSDBNode_test.cc)

target_link_libraries(tests/test-tsdbnode ${CMAKE_THREAD_LIBS_INIT})

add_executable(tests/test-streamchunk
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    StreamChunk_test.cc)

target_link_libraries(tests/test-streamchunk ${CMAKE_THREAD_LIBS_INIT})
