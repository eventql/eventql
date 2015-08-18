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

class ShopProductECommerceStatsScan : public ReportRDD {
public:

  ShopProductECommerceStatsScan(
      RefPtr<TSDBTableScanSource<ECommerceTransaction>> input,
      RefPtr<ProtoSSTableSink<ShopProductKPIs>> output,
      const ReportParams& params);

  void onInit();
  void onFinish();

  Option<String> cacheKey() const override;

protected:

  void onTransaction(const ECommerceTransaction& row);
  ShopProductKPIs* getKPIs(const String& shop_id, const String& product_id);

  RefPtr<TSDBTableScanSource<ECommerceTransaction>> input_;
  RefPtr<ProtoSSTableSink<ShopProductKPIs>> output_;
  ReportParams params_;

  OrderedMap<String, ShopProductKPIs> products_map_;
};

} // namespace zbase
