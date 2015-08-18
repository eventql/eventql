/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALYTICSQUERYREDUCER_H
#define _CM_ANALYTICSQUERYREDUCER_H
#include <stx/stdtypes.h>
#include <stx/wallclock.h>
#include <stx/csv/CSVOutputStream.h>
#include <zbase/dproc/Task.h>
#include <zbase/core/TSDBService.h>
#include <zbase/core/TSDBTableScanSpec.pb.h>
#include "zbase/AnalyticsQuery.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/AnalyticsQueryParams.pb.h"
#include "zbase/AnalyticsTableScanPlanner.h"
#include "zbase/AnalyticsQueryFactory.h"

using namespace stx;

namespace zbase {
class AnalyticsApp;

class AnalyticsQueryReducer : public dproc::RDD {
public:

  AnalyticsQueryReducer(
      const AnalyticsQuerySpec& query,
      zbase::TSDBService* tsdb,
      AnalyticsQueryFactory* factory);

  void compute(dproc::TaskContext* context) override;

  List<dproc::TaskDependency> dependencies() const;

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

  void toJSON(json::JSONOutputStream* json);
  void toCSV(CSVOutputStream* csv);

  RefPtr<AnalyticsQueryResult> queryResult();

  virtual dproc::StorageLevel storageLevel() const override {
    return dproc::StorageLevel::MEMORY;
  }

protected:
  AnalyticsQuerySpec spec_;
  AnalyticsQuery query_;
  zbase::TSDBService* tsdb_;
  AnalyticsQueryFactory* factory_;
  AnalyticsTableScan scan_;
  RefPtr<AnalyticsQueryResult> result_;
};

} // namespace zbase

#endif
