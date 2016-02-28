/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <zbase/Report.h>
#include <zbase/dproc/Task.h>
#include <csql/svalue.h>

using namespace stx;

namespace zbase {

class VTable {
public:

  virtual ~VTable() {};

  virtual Vector<String> columns() const = 0;

  //virtual size_t rowCount() const = 0;

  virtual void forEach(
      Function<bool (const Vector<csql::SValue>&)> fn) = 0;

};

class VTableSource : public ReportSource, public VTable {};

class VTableRDD : public dproc::RDD, public VTable {
public:

  //size_t rowCount() const override;

  void forEach(
      Function<bool (const Vector<csql::SValue>&)> fn) override;

  void addRow(const Vector<csql::SValue>& row);

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> src) override;

  virtual dproc::StorageLevel storageLevel() const override {
    return dproc::StorageLevel::MEMORY;
  }

protected:
  List<Vector<csql::SValue>> rows_;
};

} // namespace zbase
