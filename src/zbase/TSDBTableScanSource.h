/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TSDBTABLESCANSOURCE_H
#define _CM_TSDBTABLESCANSOURCE_H
#include "zbase/Report.h"
#include "zbase/AnalyticsTableScan.h"
#include <zbase/core/PartitionMap.h>
#include "zbase/core/TSDBTableScanSpec.pb.h"

using namespace stx;

namespace zbase {

template <typename ProtoType>
class TSDBTableScanSource : public ReportSource {
public:

  static List<RefPtr<TSDBTableScanSource>> mapStream(
      zbase::PartitionMap* tsdb,
      const String& stream,
      const UnixTime& from,
      const UnixTime& until);

  TSDBTableScanSource(
      const zbase::TSDBTableScanSpec& params,
      zbase::PartitionMap* tsdb);

  void read(dproc::TaskContext* context) override;
  AnalyticsTableScan* tableScan();

  void forEach(Function <void (const ProtoType&)> fn);

  void setRequiredFields(const Set<String>& fields);

  List<dproc::TaskDependency> dependencies() const override;

  String cacheKey() const;

protected:

  void scanWithoutIndex(dproc::TaskContext* context);
  void scanWithCSTableIndex(dproc::TaskContext* context);

  zbase::TSDBTableScanSpec params_;
  zbase::PartitionMap* tsdb_;
  AnalyticsTableScan scan_;
  Function <void (const ProtoType&)> fn_;
  Set<String> required_fields_;
};

} // namespace zbase

#include "TSDBTableScanSource_impl.h"
#endif
