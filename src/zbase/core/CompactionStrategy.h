/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <zbase/core/Table.h>
#include <zbase/core/PartitionSnapshot.h>

using namespace stx;

namespace zbase {

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
      RefPtr<Table> table,
      const String& base_path,
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
  RefPtr<Table> table_;
  String base_path_;
  size_t num_tables_soft_limit_;
  size_t num_tables_hard_limit_;
};

} // namespace zbase

