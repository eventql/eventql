/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_REPORTBUILDER_H
#define _CM_REPORTBUILDER_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "fnord-base/stdtypes.h"
#include "fnord-base/thread/taskscheduler.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include "Report.h"

using namespace fnord;

namespace cm {

class ReportBuilder : public RefCounted {
public:

  ReportBuilder();

  void addReport(RefPtr<Report> report);
  void buildAll();

protected:
  size_t buildOnce();
  size_t max_threads_;
  List<RefPtr<Report>> reports_;
};

} // namespace cm

#endif
