/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <zbase/docdb/ItemRef.h>
#include "zbase/Report.h"
#include "zbase/AnalyticsTableScanSource.h"
#include "zbase/TermInfoTableSink.h"
#include "zbase/CTRCounter.h"
#include "zbase/TermInfo.h"


using namespace stx;

namespace zbase {

/**
 * INPUT: CTR_COUNTER (key=<lang>~<search_query>)
 * OUTPUT: TERM_INFO (key=<lang>~<term>)
 */
class RelatedTermsMapper : public ReportRDD {
public:

  RelatedTermsMapper(
      RefPtr<AnalyticsTableScanSource> input,
      RefPtr<TermInfoTableSink> output);

  void onInit();
  void onFinish();

protected:
  void onQuery();

  RefPtr<AnalyticsTableScanSource> input_;
  RefPtr<TermInfoTableSink> output_table_;
  HashMap<String, HashMap<String, TermInfo>> counters_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> qstr_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> lang_col_;
};

} // namespace zbase

