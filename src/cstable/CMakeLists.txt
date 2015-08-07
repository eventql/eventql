# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

# cstable
add_library(libcstable OBJECT
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

add_executable(test-recordmaterializer
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:libcstable>
    RecordMaterializer_test.cc)

target_link_libraries(
    test-recordmaterializer
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_LIBS})

add_executable(cstable
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:libcstable>
    cstable.cc)

target_link_libraries(cstable ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

add_executable(test-cstable
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:libcstable>
    cstable_test.cc)

target_link_libraries(test-cstable  ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})
