/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALYTICSTABLESCANSOURCE_H
#define _CM_ANALYTICSTABLESCANSOURCE_H
#include "zbase/Report.h"
#include "zbase/AnalyticsTableScan.h"
#include <zbase/core/TSDBService.h>
#include <zbase/core/TSDBTableScanSpec.pb.h>

using namespace stx;

namespace cm {

class AnalyticsTableScanSource : public ReportSource {
public:

  AnalyticsTableScanSource(
      const tsdb::TSDBTableScanSpec& params,
      tsdb::TSDBService* tsdb);

  void read(dproc::TaskContext* context) override;
  AnalyticsTableScan* tableScan();

  List<dproc::TaskDependency> dependencies() const override;

protected:
  void readTables();
  tsdb::TSDBTableScanSpec params_;
  tsdb::TSDBService* tsdb_;
  String cstable_file_;
  AnalyticsTableScan scan_;
};


} // namespace cm

#endif
