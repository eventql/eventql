/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/duration.h>
#include <tsdb/Partition.h>
#include <tsdb/Partitioner.h>
#include <stx/protobuf/MessageSchema.h>
#include <tsdb/TableConfig.pb.h>

using namespace stx;

namespace tsdb {

class Table : public RefCounted{
public:

  Table(const TableDefinition& config);

  String name() const;

  String tsdbNamespace() const;

  Duration partitionSize() const;

  size_t sstableSize() const;

  size_t numShards() const;

  RefPtr<msg::MessageSchema> schema() const;

  TableDefinition config() const;

  TableStorage storage() const;

  TablePartitioner partitionerType() const;

  RefPtr<Partitioner> partitioner() const;

  void updateConfig(TableDefinition new_config);

protected:

  void loadConfig();

  mutable std::mutex mutex_;
  TableDefinition config_;
  RefPtr<msg::MessageSchema> schema_;
  RefPtr<Partitioner> partitioner_;
};

}

