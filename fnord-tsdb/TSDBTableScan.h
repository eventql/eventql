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
#include <fnord-tsdb/TSDBTableScanSpec.pb.h>
#include <fnord-tsdb/TSDBTableScanMapper.h>
#include <fnord-tsdb/TSDBTableScanReducer.h>
#include <fnord-tsdb/TSDBTableScanlet.h>

using namespace fnord;

namespace tsdb {

template <typename ScanletType>
struct TSDBTableScan {

  static RefPtr<dproc::Task> mkTask(
      const String& name,
      const Buffer& params,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb);

  static RefPtr<dproc::Task> mkTask(
      const String& name,
      const TSDBTableScanSpec& params,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb);

};

} // namespace tsdb

#include "TSDBTableScan_impl.h"
