/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_AUTOCOMPLETEREPORT_H
#define _CM_AUTOCOMPLETEREPORT_H
#include "reports/Report.h"
#include "CTRCounterTableSource.h"
#include "CTRCounterTableSink.h"
#include "CTRCounter.h"
#include "ItemRef.h"
#include "common.h"

using namespace fnord;

namespace cm {

/**
 * INPUT: CTR_COUNTER (key=<lang>~<search_query>)
 * OUTPUT: TERM_INFO (key=<lang>~<term>)
 */
class RelatedTermsReport : public Report {
public:

  RelatedTermsReport(
      RefPtr<CTRCounterTableSource> input,
      RefPtr<CTRCounterTableSink> output);

  void onInit();
  void onCTRCounter(const String& key, const CTRCounterData& c);
  void onFinish();

protected:
  RefPtr<CTRCounterTableSource> input_table_;
  RefPtr<CTRCounterTableSink> output_table_;
  HashMap<String, HashMap<String, HashMap<String, uint64_t>>> counters_;
};

} // namespace cm

#endif
