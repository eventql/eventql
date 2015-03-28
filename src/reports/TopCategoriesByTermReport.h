/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TopCategoriesByTermReport_H
#define _CM_TopCategoriesByTermReport_H
#include "reports/Report.h"
#include "CTRCounterTableSource.h"
#include "TermInfoTableSink.h"
#include "CTRCounter.h"
#include "ItemRef.h"
#include "TermInfo.h"
#include "common.h"

using namespace fnord;

namespace cm {

/**
 * INPUT: CTR_COUNTER (key=<lang>~<search_query>)
 * OUTPUT: TERM_INFO (key=<lang>~<term>)
 */
class TopCategoriesByTermReport : public Report {
public:

  TopCategoriesByTermReport(
      RefPtr<CTRCounterTableSource> input,
      RefPtr<TermInfoTableSink> output,
      const String cat_prefix = "");

  void onInit();
  void onCTRCounter(const String& key, const CTRCounterData& c);
  void onFinish();

protected:
  RefPtr<CTRCounterTableSource> input_table_;
  RefPtr<TermInfoTableSink> output_table_;
  String cat_prefix_;
  HashMap<String, HashMap<String, TermInfo>> counters_;
};

} // namespace cm

#endif
