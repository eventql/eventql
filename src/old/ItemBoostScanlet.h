/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ITEMBOOSTSCANLET_H_
#define _CM_ITEMBOOSTSCANLET_H_
#include "zbase/AnalyticsTableScanlet.h"
#include "zbase/CTRCounter.h"
#include "ItemBoost.pb.h"

namespace zbase {

class ItemBoostScanlet : public AnalyticsTableScanlet<
    ItemBoostParams,
    ItemBoostResult> {
public:

  static String name() {
    return "ItemBoost";
  }

  static void mergeResults(ItemBoostResult* dst, ItemBoostResult* src);

  ItemBoostScanlet(const ItemBoostParams& params);

  void onQueryItem();
  void getResult(ItemBoostResult* result) override;

protected:
  RefPtr<AnalyticsTableScan::ColumnRef> itemid_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicked_col_;
  HashMap<String, CTRCounterData> counters_;
};

}
#endif
