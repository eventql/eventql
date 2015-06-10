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
#include <fnord-tsdb/TSDBTableScanMapperParams.pb.h>

using namespace fnord;

namespace tsdb {

template <typename ScanletType>
class TSDBTableScanMapper : public dproc::RDD {
public:

  TSDBTableScanMapper(
      const TSDBTableScanMapperParams& params,
      RefPtr<ScanletType> scanlet);

protected:
  RefPtr<ScanletType> scanlet_;
};

} // namespace tsdb
