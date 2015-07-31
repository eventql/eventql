/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <tsdb/PartitionSnapshot.h>

using namespace stx;

namespace tsdb {
class Partition;

struct PartitionReader : public RefCounted {

  PartitionReader(RefPtr<PartitionSnapshot> head);

  void fetchRecords(Function<void (const Buffer& record)> fn);

  void fetchRecordsWithSampling(
      size_t sample_modulo,
      size_t sample_index,
      Function<void (const Buffer& record)> fn);

protected:
  RefPtr<PartitionSnapshot> snap_;
};

} // namespace tdsb

