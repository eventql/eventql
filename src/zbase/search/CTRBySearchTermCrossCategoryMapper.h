/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYSEARCHTERMCROSSCATEGORY_H
#define _CM_CTRBYSEARCHTERMCROSSCATEGORY_H
#include "zbase/Report.h"
#include "zbase/AnalyticsTableScanSource.h"
#include "zbase/CTRCounterTableSink.h"
#include "zbase/CTRCounter.h"
#include <zbase/docdb/ItemRef.h>


using namespace stx;

namespace zbase {

/**
 * INPUT: JOINED_QUERY
 * OUTPUT: CTR_COUNTER (key=<lang>~<term>~<category>)
 */
class CTRBySearchTermCrossCategoryMapper : public ReportRDD {
public:

  CTRBySearchTermCrossCategoryMapper(
      RefPtr<AnalyticsTableScanSource> input,
      RefPtr<CTRCounterTableSink> output,
      const String& category_field);

  void onInit();
  void onFinish();

protected:
  void onQuery();
  void onQueryItem();

  RefPtr<AnalyticsTableScanSource> input_;
  RefPtr<CTRCounterTableSink> ctr_table_;
  String field_;
  HashMap<String, CTRCounterData> counters_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> page_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> qstr_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> lang_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> itemid_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicked_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> category_col_;
  Vector<String> cur_terms_;
  String cur_lang_;
};

} // namespace zbase

#endif
