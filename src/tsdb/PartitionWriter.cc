/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/io/fileutil.h>
#include <tsdb/Partition.h>
#include <tsdb/PartitionWriter.h>
#include <stx/logging.h>
#include <sstable/SSTableWriter.h>

using namespace stx;

namespace tsdb {

PartitionWriter::PartitionWriter(
    PartitionSnapshotRef* head) :
    head_(head) {}

} // namespace tdsb
