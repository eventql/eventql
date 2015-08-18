/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALYTICSQUERYMAPPER_H
#define _CM_ANALYTICSQUERYMAPPER_H
#include <stx/stdtypes.h>
#include <stx/wallclock.h>
#include <dproc/Task.h>
#include <zbase/core/TSDBService.h>
#include <zbase/core/TSDBTableScanSpec.pb.h>
#include "zbase/AnalyticsQueryParams.pb.h"
#include "zbase/AnalyticsQueryFactory.h"

using namespace stx;

namespace zbase {
class AnalyticsApp;

class AnalyticsQueryMapper : public dproc::RDD {
public:

  AnalyticsQueryMapper(
      const zbase::TSDBTableScanSpec& params,
      zbase::PartitionMap* pmap,
      AnalyticsQueryFactory* factory);

  void compute(dproc::TaskContext* context) override;

  //List<dproc::TaskDependency> dependencies() const override;

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

  Option<String> cacheKey() const override;

  RefPtr<AnalyticsQueryResult> queryResult() const;

protected:
  zbase::TSDBTableScanSpec params_;
  zbase::PartitionMap* pmap_;
  AnalyticsQuerySpec spec_;
  AnalyticsQuery query_;
  AnalyticsQueryFactory* factory_;
  AnalyticsTableScan scan_;
  RefPtr<AnalyticsQueryResult> result_;
};

} // namespace zbase

#endif
