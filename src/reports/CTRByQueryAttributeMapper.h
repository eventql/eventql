/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYQUERYATTRIBUTEMAPPER_H
#define _CM_CTRBYQUERYATTRIBUTEMAPPER_H
#include "reports/Report.h"
#include "JoinedQuery.h"
#include "JoinedQueryTableSource.h"
#include "CTRCounterTableSink.h"
#include "CTRCounter.h"
#include "ItemRef.h"
#include "common.h"

using namespace fnord;

namespace cm {

/**
 * INPUT: JOINED_QUERY
 * OUTPUT: CTR_COUNTER (key=<attr>)
 */
class CTRByQueryAttributeMapper : public Report {
public:

  CTRByQueryAttributeMapper(
      RefPtr<JoinedQueryTableSource> input,
      RefPtr<CTRCounterTableSink> output,
      const String& attribute,
      ItemEligibility eligibility);

  void onInit();
  void onJoinedQuery(const JoinedQuery& q);
  void onFinish();

protected:
  RefPtr<JoinedQueryTableSource> joined_queries_;
  RefPtr<CTRCounterTableSink> ctr_table_;
  String attribute_;
  ItemEligibility eligibility_;
  HashMap<String, CTRCounterData> counters_;
};

} // namespace cm

#endif
