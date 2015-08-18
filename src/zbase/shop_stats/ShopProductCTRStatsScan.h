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

namespace zbase {

class ShopProductCTRStatsScan : public ReportRDD {
public:

  ShopProductCTRStatsScan(
      RefPtr<TSDBTableScanSource<JoinedSession>> input,
      RefPtr<ProtoSSTableSink<ShopProductKPIs>> output,
      const ReportParams& params);

  void onInit();
  void onFinish();

  Option<String> cacheKey() const override;

protected:

  void onSession(const JoinedSession& row);
  ShopProductKPIs* getKPIs(const String& shop_id, const String& product_id);

  RefPtr<TSDBTableScanSource<JoinedSession>> input_;
  RefPtr<ProtoSSTableSink<ShopProductKPIs>> output_;
  ReportParams params_;

  OrderedMap<String, ShopProductKPIs> products_map_;
};

} // namespace zbase
