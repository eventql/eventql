# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

# sstable
add_library(sstable STATIC
    cursor.cc
    MetaPage.cc
    fileheaderreader.cc
    fileheaderwriter.cc
    index.cc
    indexprovider.cc
    rowoffsetindex.cc
    RowWriter.cc
    sstablereader.cc
    sstablerepair.cc
    SSTableEditor.cc
    SSTableScan.cc
    SSTableColumnSchema.cc
    SSTableColumnReader.cc
    SSTableColumnWriter.cc
    SSTableWriter.cc)

add_executable(fn-sstablescan fn-sstablescan.cc)
target_link_libraries(fn-sstablescan stx-base sstable)

add_executable(test-sstable sstable_test.cc)
target_link_libraries(test-sstable stx-base sstable)
