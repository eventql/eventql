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
#include "zbase/ECommerceTransaction.pb.h"

using namespace stx;

namespace zbase {

class ShopECommerceStatsScan : public ReportRDD {
public:

  ShopECommerceStatsScan(
      RefPtr<TSDBTableScanSource<ECommerceTransaction>> input,
      RefPtr<ProtoSSTableSink<ShopKPIs>> output,
      const ReportParams& params);

  void onInit();
  void onFinish();

  Option<String> cacheKey() const override;

protected:

  void onTransaction(const ECommerceTransaction& row);
  ShopKPIs* getKPIs(const String& shop_id, const UnixTime& time);

  RefPtr<TSDBTableScanSource<ECommerceTransaction>> input_;
  RefPtr<ProtoSSTableSink<ShopKPIs>> output_;
  ReportParams params_;

  HashMap<String, ShopKPIs> shops_map_;
  Option<Duration> time_window_;
};

} // namespace zbase
