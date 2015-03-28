/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYSEARCHQUERY_H
#define _CM_CTRBYSEARCHQUERY_H
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
 * OUTPUT: CTR_COUNTER (key=<lang>~<search_query>)
 */
class CTRBySearchQueryMapper : public Report {
public:

  CTRBySearchQueryMapper(
      RefPtr<JoinedQueryTableSource> input,
      RefPtr<CTRCounterTableSink> output,
      ItemEligibility eligibility,
      RefPtr<fts::Analyzer> analyzer);

  void onInit();
  void onJoinedQuery(const JoinedQuery& q);
  void onFinish();

protected:
  RefPtr<JoinedQueryTableSource> joined_queries_;
  RefPtr<CTRCounterTableSink> ctr_table_;
  ItemEligibility eligibility_;
  RefPtr<fts::Analyzer> analyzer_;
  HashMap<String, CTRCounterData> counters_;
};

} // namespace cm

#endif
