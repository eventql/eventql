/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "zbase/core/TSDBTableScan.h"
#include "zbase/Report.h"
#include "zbase/VTable.h"
#include "zbase/ShopStats.pb.h"
#include "zbase/ProtoSSTableSource.h"
#include "zbase/shop_stats/ShopCTRStatsScan.h"
#include "zbase/shop_stats/ShopKPITable.h"
#include "zbase/JoinedSession.pb.h"

using namespace stx;

namespace cm {

class ShopKPIDashboardQuery :  public dproc::BlobRDD {
public:

  ShopKPIDashboardQuery(
      const ReportParams& params,
      tsdb::TSDBService* tsdb);

  RefPtr<VFSFile> computeBlob(dproc::TaskContext* ctx) override;
  List<dproc::TaskDependency> dependencies() const override;

protected:
  ShopKPITable source_;
};

} // namespace cm
