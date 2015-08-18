/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <inventory/ItemRef.h>
#include "zbase/Report.h"
#include "zbase/CTRCounterTableSource.h"
#include "zbase/TermInfoTableSink.h"
#include "zbase/CTRCounter.h"
#include "zbase/TermInfo.h"


using namespace stx;

namespace cm {

/**
 * INPUT: CTR_COUNTER (key=<lang>~<term>~category)
 * OUTPUT: TERM_INFO (key=<lang>~<term>)
 */
class TopCategoriesByTermMapper : public ReportRDD {
public:

  TopCategoriesByTermMapper(
      RefPtr<CTRCounterTableSource> input,
      RefPtr<TermInfoTableSink> output,
      const String& cat_prefix = "");

  void onInit();
  void onCTRCounter(const String& key, const CTRCounterData& c);
  void onFinish();

protected:
  RefPtr<CTRCounterTableSource> input_table_;
  RefPtr<TermInfoTableSink> output_table_;
  String cat_prefix_;
  HashMap<String, TermInfo> counters_;
};

} // namespace cm
