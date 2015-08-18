/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ITEMBOOSTMAPPER__H
#define _CM_ITEMBOOSTMAPPER__H
#include "zbase/Report.h"
#include "zbase/AnalyticsTableScanSource.h"
#include "zbase/ProtoSSTableSink.h"
#include "zbase/ProtoCRDT.h"
#include "zbase/ItemBoost.pb.h"


using namespace stx;

namespace zbase {

class ItemBoostMapper : public ReportRDD {
public:

  ItemBoostMapper(
      RefPtr<AnalyticsTableScanSource> input,
      RefPtr<ProtoSSTableSink<ItemBoostRow>> output);

  void onInit();
  void onFinish();

protected:
  void onQuery();
  void onQueryItem();

  struct ScratchData {
    ScratchData() : num_views(0), num_clicks(0) {}
    uint64_t num_views;
    uint64_t num_clicks;
    HashMap<String, uint64_t> terms;
  };

  RefPtr<AnalyticsTableScanSource> input_;
  RefPtr<ProtoSSTableSink<ItemBoostRow>> output_;
  HashMap<String, ScratchData> scratch_;
  RefPtr<AnalyticsTableScan::ColumnRef> qstr_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> itemid_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> pos_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicked_col_;
  Vector<String> cur_terms_;
};

} // namespace zbase

#endif
