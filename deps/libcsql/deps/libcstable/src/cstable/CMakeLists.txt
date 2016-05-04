# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

# cstable
add_library(cstable STATIC
    cstable.cc
    columns/BitPackedIntPageWriter.cc
    columns/UInt64PageWriter.cc
    columns/UInt64PageReader.cc
    columns/UnsignedIntColumnWriter.cc
    columns/UnsignedIntColumnReader.cc
    columns/v1/BitPackedIntColumnReader.cc
    columns/v1/BitPackedIntColumnWriter.cc
    columns/v1/BooleanColumnReader.cc
    columns/v1/BooleanColumnWriter.cc
    columns/v1/ColumnWriter.cc
    columns/v1/DoubleColumnReader.cc
    columns/v1/DoubleColumnWriter.cc
    columns/v1/LEB128ColumnReader.cc
    columns/v1/LEB128ColumnWriter.cc
    columns/v1/StringColumnReader.cc
    columns/v1/StringColumnWriter.cc
    columns/v1/UInt32ColumnReader.cc
    columns/v1/UInt32ColumnWriter.cc
    columns/v1/UInt64ColumnReader.cc
    columns/v1/UInt64ColumnWriter.cc
    io/PageManager.cc
    io/PageIndex.cc
    ColumnWriter.cc
    ColumnReader.cc
    CSTableReader.cc
    CSTableWriter.cc
    RecordMaterializer.cc
    RecordShredder.cc
    TableSchema.cc
    CSTableWriter.cc)

add_executable(test-cstable cstable_test.cc)
target_link_libraries(test-cstable cstable stx-protobuf stx-json stx-base)

add_executable(cstable-tool cstable_tool.cc)
target_link_libraries(cstable-tool cstable stx-protobuf stx-json stx-base)
