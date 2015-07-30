# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

# sstable
add_library(sstable OBJECT
    cursor.cc
    fileheaderreader.cc
    fileheaderwriter.cc
    index.cc
    indexprovider.cc
    rowoffsetindex.cc
    sstablereader.cc
    sstablerepair.cc
    SSTableEditor.cc
    SSTableScan.cc
    SSTableColumnSchema.cc
    SSTableColumnReader.cc
    SSTableColumnWriter.cc)

add_executable(fn-sstablescan
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:sstable>
    fn-sstablescan.cc)

target_link_libraries(fn-sstablescan ${CMAKE_THREAD_LIBS_INIT})
