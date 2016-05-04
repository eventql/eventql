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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/option.h>
#include <eventql/core/PartitionSnapshot.h>
#include <eventql/core/PartitionReader.h>

using namespace stx;

namespace zbase {
class Partition;

class StaticPartitionReader : public PartitionReader {
public:

  StaticPartitionReader(
      RefPtr<Table> table,
      RefPtr<PartitionSnapshot> head);

  void fetchRecords(
      const Set<String>& required_columns,
      Function<void (const msg::MessageObject& record)> fn) override;

  SHA1Hash version() const override;

protected:
  RefPtr<Table> table_;
};

} // namespace tdsb

