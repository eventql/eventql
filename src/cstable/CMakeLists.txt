# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

# cstable
add_library(cstable OBJECT
    CSTableBuilder.cc
    CSTableReader.cc
    CSTableWriter.cc
    ColumnWriter.cc
    BooleanColumnReader.cc
    BooleanColumnWriter.cc
    UInt32ColumnReader.cc
    UInt32ColumnWriter.cc
    UInt64ColumnReader.cc
    UInt64ColumnWriter.cc
    LEB128ColumnReader.cc
    LEB128ColumnWriter.cc
    BitPackedIntColumnReader.cc
    BitPackedIntColumnWriter.cc
    RecordMaterializer.cc
    StringColumnReader.cc
    StringColumnWriter.cc)

add_executable(tests/test-recordmaterializer
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:cstable>
    RecordMaterializer_test.cc)

target_link_libraries(
    tests/test-recordmaterializer
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_LIBS})
