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
#include "zbase/ProtoSSTableSource.h"
#include "zbase/ProtoSSTableSink.h"

using namespace stx;

namespace zbase {

template <typename T>
class ProtoSSTableMergeReducer : public ReportRDD {
public:

  ProtoSSTableMergeReducer(
      RefPtr<ProtoSSTableSource<T>> input,
      RefPtr<ProtoSSTableSink<T>> output,
      Function<void (T*, const T&)> merge_fn);

  void onInit();
  void onRow(const String& key, const T& value);
  void onFinish();

protected:
  RefPtr<ProtoSSTableSource<T>> input_table_;
  RefPtr<ProtoSSTableSink<T>> output_table_;
  HashMap<String, T> map_;
  Function<void (T*, const T&)> merge_;
};

} // namespace zbase

#include "ProtoSSTableMergeReducer_impl.h"
#endif
