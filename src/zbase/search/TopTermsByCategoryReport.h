/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "zbase/Report.h"
#include "zbase/CTRCounterTableSource.h"
#include "zbase/CSVSink.h"
#include "zbase/CTRCounter.h"
#include <zbase/docdb/ItemRef.h>


using namespace stx;

namespace zbase {

/**
 * INPUT: CTR_COUNTER (key=<lang>~<term>~category)
 * OUTPUT: CSV
 */
class TopTermsByCategoryReport : public ReportRDD {
public:

  TopTermsByCategoryReport(
      RefPtr<CTRCounterTableSource> input,
      RefPtr<CSVSink> output);

  void onInit();
  void onCTRCounter(const String& key, const CTRCounterData& c);
  void onFinish();

protected:
  RefPtr<CTRCounterTableSource> input_table_;
  RefPtr<CSVSink> output_table_;
  HashMap<String, Vector<Pair<String, uint64_t>>> counters_;
};

} // namespace zbase
