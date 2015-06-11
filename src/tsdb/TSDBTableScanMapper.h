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
#include <fnord-tsdb/TSDBClient.h>
#include <fnord-tsdb/TSDBTableScanlet.h>
#include <fnord-tsdb/TSDBTableScanMapperParams.pb.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-dproc/Task.h>

using namespace fnord;

namespace tsdb {

template <typename ScanletType>
class TSDBTableScanMapper : public dproc::RDD {
public:
  typedef typename ScanletType::ResultType ResultType;

  TSDBTableScanMapper(
      const String& name,
      const TSDBTableScanSpec& params,
      RefPtr<ScanletType> scanlet,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb);

  void compute(dproc::TaskContext* context);
  List<dproc::TaskDependency> dependencies() const;

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

  ResultType* result();

protected:

  void onRow(const Buffer& row);

  void scanWithoutIndex(dproc::TaskContext* context);
  void scanWithCSTableIndex(dproc::TaskContext* context);

  TSDBTableScanSpec params_;
  RefPtr<ScanletType> scanlet_;
  RefPtr<msg::MessageSchema> schema_;
  TSDBClient* tsdb_;
};

} // namespace tsdb

#include "TSDBTableScanMapper_impl.h"
