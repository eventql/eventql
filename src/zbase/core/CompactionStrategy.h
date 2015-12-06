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

  virtual bool needsCompaction(
      const Vector<LSMTableRef>& tables) = 0;

};

class SimpleCompactionStrategy : public CompactionStrategy {
public:

  SimpleCompactionStrategy(
      RefPtr<Table> table,
      const String& base_path);

  bool compact(
      const Vector<LSMTableRef>& input,
      Vector<LSMTableRef>* output) override;

  bool needsCompaction(
      const Vector<LSMTableRef>& tables) override;

protected:
  RefPtr<Table> table_;
  String base_path_;
};

} // namespace zbase

