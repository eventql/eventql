/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/PartitionReader.h>

using namespace stx;

PartitionReader::PartitionReader(
    RefPtr<PartitionSnapshot> head) :
    snap_(head) {}

void PartitionReader::fetchRecords(Function<void (const Buffer& record)> fn) {
  fetchRecordsWithSampling(
      0,
      0,
      fn);
}

void PartitionReader::fetchRecordsWithSampling(
    size_t sample_modulo,
    size_t sample_index,
    Function<void (const Buffer& record)> fn);

} // namespace tdsb

