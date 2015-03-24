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
class CTRCounterMerge : public Report {
public:
  void onEvent(ReportEventType type, void* ev) override;
protected:
  HashMap<String, CTRCounterData> counters_;
};

} // namespace cm

#endif
