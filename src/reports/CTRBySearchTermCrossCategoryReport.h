/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYSEARCHTERMCROSSCATEGORY_H
#define _CM_CTRBYSEARCHTERMCROSSCATEGORY_H
#include "reports/Report.h"
#include "JoinedQuery.h"
#include "JoinedQueryTableSource.h"
#include "CTRCounterTableSink.h"
#include "CTRCounter.h"
#include "ItemRef.h"
#include "IndexReader.h"
#include "common.h"

using namespace fnord;

namespace cm {

/**
 * INPUT: JOINED_QUERY
 * OUTPUT: CTR_COUNTER (key=<lang>~<search_query>)
 */
class CTRBySearchTermCrossCategoryReport : public Report {
public:

  CTRBySearchTermCrossCategoryReport(
      RefPtr<JoinedQueryTableSource> input,
      RefPtr<CTRCounterTableSink> output,
      const String& category_field,
      ItemEligibility eligibility,
      RefPtr<fts::Analyzer> analyzer,
      RefPtr<IndexReader> index);

  void onInit();
  void onJoinedQuery(const JoinedQuery& q);
  void onFinish();

protected:
  RefPtr<JoinedQueryTableSource> joined_queries_;
  RefPtr<CTRCounterTableSink> ctr_table_;
  ItemEligibility eligibility_;
  String field_;
  RefPtr<fts::Analyzer> analyzer_;
  RefPtr<IndexReader> index_;
  HashMap<String, CTRCounterData> counters_;
};

} // namespace cm

#endif
