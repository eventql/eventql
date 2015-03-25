/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYPOSITIONREPORT_H
#define _CM_CTRBYPOSITIONREPORT_H
#include "reports/Report.h"
#include "JoinedQuery.h"
#include "JoinedQueryTableSource.h"
#include "CTRCounterSSTableSink.h"
#include "CTRCounter.h"
#include "ItemRef.h"
#include "common.h"

using namespace fnord;

namespace cm {

/**
 * INPUT: JOINED_QUERY
 * OUTPUT: CTR_COUNTER (key=<lang>~<testgroup>~<devicetype>~<posi>)
 */
class CTRByPositionReport : public Report {
public:

  CTRByPositionReport(
      RefPtr<JoinedQueryTableSource> input,
      RefPtr<CTRCounterSSTableSink> output,
      ItemEligibility eligibility);

  void onInit();
  void onJoinedQuery(const JoinedQuery& q);
  void onFinish();

protected:
  RefPtr<JoinedQueryTableSource> joined_queries_;
  RefPtr<CTRCounterSSTableSink> ctr_table_;
  ItemEligibility eligibility_;
  HashMap<String, CTRCounterData> counters_;
};

} // namespace cm

#endif
