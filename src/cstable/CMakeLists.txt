# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

# cstable
add_library(cstable STATIC
    BitPackedIntColumnReader.cc
    BitPackedIntColumnWriter.cc
    BooleanColumnReader.cc
    BooleanColumnWriter.cc
    CSTableBuilder.cc
    CSTableReader.cc
    CSTableWriter.cc
    ColumnWriter.cc
    DoubleColumnReader.cc
    DoubleColumnWriter.cc
    LEB128ColumnReader.cc
    LEB128ColumnWriter.cc
    RecordMaterializer.cc
    StringColumnReader.cc
    StringColumnWriter.cc
    UInt32ColumnReader.cc
    UInt32ColumnWriter.cc
    UInt64ColumnReader.cc
    UInt64ColumnWriter.cc)

add_executable(test-cstable-recordmaterializer RecordMaterializer_test.cc)
target_link_libraries(test-cstable-recordmaterializer cstable stx-protobuf stx-json stx-base)

add_executable(test-cstable cstable_test.cc)
target_link_libraries(test-cstable cstable stx-protobuf stx-json stx-base)

add_executable(cstable-tool cstable.cc)
target_link_libraries(cstable-tool cstable stx-protobuf stx-json stx-base)
