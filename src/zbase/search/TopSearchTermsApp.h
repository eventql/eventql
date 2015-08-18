/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TOPSEARCHTERMSAPP_H
#define _CM_TOPSEARCHTERMSAPP_H
#include "dproc/Application.h"
#include "zbase/AnalyticsTableScanSource.h"
#include "zbase/CTRCounterMergeReducer.h"
#include "zbase/CTRBySearchTermCrossCategoryMapper.h"
#include "zbase/TopTermsByCategoryReport.h"
#include "AnalyticsTableScanParams.pb.h"


using namespace stx;

namespace cm {

class TopSearchTermsApp : public dproc::DefaultApplication {
public:

  TopSearchTermsApp(tsdb::TSDBClient* tsdb);

protected:

  tsdb::TSDBClient* tsdb_;
};

} // namespace cm

#endif
