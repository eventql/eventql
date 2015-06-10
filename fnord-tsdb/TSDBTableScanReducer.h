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
#include "fnord-base/stdtypes.h"
#include <fnord-tsdb/TSDBTableScanlet.h>
#include <fnord-tsdb/TSDBTableScanReducerParams.pb.h>

using namespace fnord;

namespace tsdb {

template <typename ScanletType>
class TSDBTableScanReducer : public dproc::RDD {
public:
  typedef typename ScanletType::ResultType ResultType;

  TSDBTableScanReducer(
      const String& name,
      const TSDBTableScanSpec& params,
      RefPtr<ScanletType> scanlet,
      TSDBClient* tsdb);

  void compute(dproc::TaskContext* context);
  List<dproc::TaskDependency> dependencies() const;

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

  ResultType* result();

protected:
  String name_;
  TSDBTableScanSpec params_;
  RefPtr<ScanletType> scanlet_;
  ResultType result_;
  TSDBClient* tsdb_;
};

} // namespace tsdb

#include "TSDBTableScanReducer_impl.h"
