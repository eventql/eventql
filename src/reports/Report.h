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
  END,
  JOINED_QUERY,
  CTR_COUNTER
};

typedef Option<Pair<DateTime, DateTime>> ReportEventTime;

class ReportSource : public RefCounted {
public:
  virtual ~ReportSource() {}
  virtual Set<String> inputFiles() = 0;
};

class ReportSink : public RefCounted {
public:
  virtual ~ReportSink() {}
  virtual Set<String> outputFiles() = 0;
};

class Report : public RefCounted {
public:

  virtual ~Report() {}

  List<RefPtr<ReportSource>> inputs();
  List<RefPtr<ReportSink>> outputs();

  void addInput(RefPtr<ReportSource> input);
  void addOutput(RefPtr<ReportSink> output);

protected:
  List<RefPtr<ReportSource>> inputs_;
  List<RefPtr<ReportSink>> outputs_;
};

} // namespace cm

#endif
