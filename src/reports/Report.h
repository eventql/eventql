/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_REPORT_H
#define _CM_REPORT_H
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
#include "FeatureIndex.h"
#include "DocStore.h"
#include "IndexRequest.h"
#include "FeatureIndexWriter.h"
#include "ItemRef.h"

using namespace fnord;

namespace cm {

enum class ReportEventType {
  BEGIN,
  JOINED_QUERY,
  END
};

class Report : public RefCounted {
public:

  void addReport(RefPtr<Report> report);

  virtual void onEvent(ReportEventType type, void* ev) = 0;

  virtual Set<String> inputFiles();
  virtual Set<String> outputFiles();

protected:

  void emitEvent(ReportEventType type, void* ev);

  List<RefPtr<Report>> children_;
};

} // namespace cm

#endif
