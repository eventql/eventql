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
#include "zbase/TSDBTableScanSource.h"
#include "zbase/ProtoSSTableSink.h"
#include "zbase/JoinedSession.pb.h"

using namespace stx;

namespace cm {

class ShopCTRStatsScan : public ReportRDD {
public:

  ShopCTRStatsScan(
      RefPtr<TSDBTableScanSource<JoinedSession>> input,
      RefPtr<ProtoSSTableSink<ShopKPIs>> output,
      const ReportParams& params);

  void onInit();
  void onFinish();

  Option<String> cacheKey() const override;

protected:

  void onSession(const JoinedSession& row);
  ShopKPIs* getKPIs(const String& shop_id, const UnixTime& time);

  RefPtr<TSDBTableScanSource<JoinedSession>> input_;
  RefPtr<ProtoSSTableSink<ShopKPIs>> output_;
  ReportParams params_;

  HashMap<String, ShopKPIs> shops_map_;
  Option<Duration> time_window_;
};

} // namespace cm
