/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/db/Table.h>
#include <eventql/db/PartitionSnapshot.h>

#include "eventql/eventql.h"

namespace eventql {

class CompactionStrategy : public RefCounted {
public:

  virtual bool compact(
      const Vector<LSMTableRef>& input,
      Vector<LSMTableRef>* output) = 0;

  /**
   * Should return true if this partition needs a compaction at some point
   * in the future and false otherwise
   */
  virtual bool needsCompaction(
      const Vector<LSMTableRef>& tables) = 0;

  /**
   * Should return true if this partition needs a compaction immediately,
   * (e.g. because we are filling up lsm update tables with updates faster
   * than we can get around oto merge them) and false otherwise
   */
  virtual bool needsUrgentCompaction(
      const Vector<LSMTableRef>& tables) = 0;

};

class SimpleCompactionStrategy : public CompactionStrategy {
public:
  static const size_t kDefaultNumTablesSoftLimit = 3;
  static const size_t kDefaultNumTablesHardLimit = 8;

  SimpleCompactionStrategy(
      RefPtr<Partition> partition,
      LSMTableIndexCache* idx_cache,
      size_t num_tables_soft_limit = kDefaultNumTablesSoftLimit,
      size_t num_tables_hard_limit = kDefaultNumTablesHardLimit);

  bool compact(
      const Vector<LSMTableRef>& input,
      Vector<LSMTableRef>* output) override;

  bool needsCompaction(
      const Vector<LSMTableRef>& tables) override;

  bool needsUrgentCompaction(
      const Vector<LSMTableRef>& tables) override;

protected:
  RefPtr<Partition> partition_;
  LSMTableIndexCache* idx_cache_;
  size_t num_tables_soft_limit_;
  size_t num_tables_hard_limit_;
};

} // namespace eventql

