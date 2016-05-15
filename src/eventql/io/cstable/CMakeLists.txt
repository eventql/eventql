# Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
# Authors:
#   - Paul Asmuth <paul@zscale.io>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License ("the license") as
# published by the Free Software Foundation, either version 3 of the License,
# or any later version.
#
# In accordance with Section 7(e) of the license, the licensing of the Program
# under the license does not imply a trademark license. Therefore any rights,
# title and interest in our trademarks remain entirely with us.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the license for more details.
#
# You can be released from the requirements of the license by purchasing a
# commercial license. Buying such a license is mandatory as soon as you develop
# commercial activities involving this program without disclosing the source
# code of your own applications

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
