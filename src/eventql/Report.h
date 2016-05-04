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
#include "eventql/util/stdtypes.h"
#include "eventql/util/thread/taskscheduler.h"
#include "eventql/util/exception.h"
#include "eventql/util/csv/CSVOutputStream.h"
#include "eventql/util/uri.h"
#include "eventql/util/json/json.h"
#include "eventql/dproc/TaskSpec.pb.h"
#include "eventql/dproc/Task.h"
#include "eventql/dproc/BlobRDD.h"
#include <eventql/docdb/ItemRef.h>
#include <eventql/sql/svalue.h>
#include "eventql/ReportParams.pb.h"

using namespace stx;

#define pb_incr(P, F, V) (P).set_##F((P).F() + (V));
#define pb_nmerge(D, S, F) (D).set_##F((D).F() + (S).F());

namespace zbase {

class ReportSource : public RefCounted {
public:
  virtual ~ReportSource() {}
  virtual void read(dproc::TaskContext* context) = 0;
  virtual List<dproc::TaskDependency> dependencies() const = 0;
};

class ReportSink : public RefCounted {
public:
  virtual ~ReportSink() {}
  virtual void open() = 0;
  virtual RefPtr<VFSFile> finalize() = 0;
  virtual String contentType() const = 0;
};


// fixpaul rename to reportlet??
class ReportRDD : public dproc::RDD {
public:

  ReportRDD(RefPtr<ReportSource> input, RefPtr<ReportSink> output);

  void compute(dproc::TaskContext* context);

  String contentType() const override;

  List<dproc::TaskDependency> dependencies() const override;
  Option<String> cacheKey() const override;

  virtual void onInit();
  virtual void onFinish();

  void setCacheKey(const String& key);

  RefPtr<ReportSource> input();
  RefPtr<ReportSink> output();

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

protected:
  RefPtr<ReportSource> input_;
  RefPtr<ReportSink> output_;
  RefPtr<VFSFile> result_;
  Option<String> cache_key_;
};

void rowToJSON(
    const Vector<String>& columns,
    const Vector<csql::SValue>& row,
    json::JSONOutputStream* json);

} // namespace zbase

#endif
