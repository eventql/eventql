/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRCOUNTERMERGE_H
#define _CM_CTRCOUNTERMERGE_H
#include "reports/Report.h"
#include "reports/CTRCounterTableSink.h"
#include "reports/CTRCounterTableSource.h"
#include "JoinedQuery.h"
#include "CTRCounter.h"
#include "ItemRef.h"
#include "common.h"

using namespace fnord;

namespace cm {

/**
 * INPUT: CTR_COUNTER
 * OUTPUT: CTR_COUNTER
 */
class CTRCounterMergeReducer : public Report {
public:

  CTRCounterMergeReducer(
      RefPtr<CTRCounterTableSource> input,
      RefPtr<CTRCounterTableSink> output);

  void onInit();
  void onCTRCounter(const String& key, const CTRCounterData& c);
  void onFinish();

protected:
  RefPtr<CTRCounterTableSource> input_table_;
  RefPtr<CTRCounterTableSink> output_table_;
  HashMap<String, CTRCounterData> counters_;
};

} // namespace cm

#endif
