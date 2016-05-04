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
#include "eventql/Report.h"
#include "eventql/CTRCounterTableSink.h"
#include "eventql/CTRCounterTableSource.h"
#
#include "eventql/CTRCounter.h"
#include <eventql/docdb/ItemRef.h>


using namespace stx;

namespace zbase {

/**
 * INPUT: CTR_COUNTER
 * OUTPUT: CTR_COUNTER
 */
class CTRCounterMergeReducer : public ReportRDD {
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

} // namespace zbase

#endif
