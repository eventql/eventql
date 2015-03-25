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
  virtual void read() = 0;
  virtual Set<String> inputFiles() = 0;
};

class ReportSink : public RefCounted {
public:
  virtual ~ReportSink() {}
  virtual void open() = 0;
  virtual Set<String> outputFiles() = 0;
  virtual void close() = 0;
};

class Report : public RefCounted {
public:

  Report(RefPtr<ReportSource> input, RefPtr<ReportSink> output);
  virtual ~Report();

  virtual void onInit();
  virtual void onFinish();

  RefPtr<ReportSource> input();
  RefPtr<ReportSink> output();

protected:
  RefPtr<ReportSource> input_;
  RefPtr<ReportSink> output_;
};

} // namespace cm

#endif
