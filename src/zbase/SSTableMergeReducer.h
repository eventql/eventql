/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_SSTABLEMERGEREDUCER_H
#define _CM_SSTABLEMERGEREDUCER_H
#include "zbase/Report.h"
#include "zbase/SSTableSource.h"
#include "zbase/SSTableSink.h"

using namespace stx;

namespace zbase {

template <typename T>
class SSTableMergeReducer : public ReportRDD {
public:

  SSTableMergeReducer(
      RefPtr<SSTableSource<T>> input,
      RefPtr<SSTableSink<T>> output);

  void onInit();
  void onRow(const String& key, const T& value);
  void onFinish();

protected:
  RefPtr<SSTableSource<T>> input_table_;
  RefPtr<SSTableSink<T>> output_table_;
  HashMap<String, T> counters_;
};

} // namespace zbase

#include "SSTableMergeReducer_impl.h"
#endif
