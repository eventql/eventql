/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TERMINFOMERGE_H
#define _CM_TERMINFOMERGE_H
#include "zbase/Report.h"
#include "CTRCounterTableSource.h"
#include "TermInfoTableSink.h"
#include "TermInfoTableSource.h"
#include "TermInfo.h"
#include <inventory/ItemRef.h>


using namespace stx;

namespace zbase {

/**
 * INPUT: TERM_INFO (key=<lang>~<term>)
 * OUTPUT: TERM_INFO (key=<lang>~<term>)
 */
class TermInfoMergeReducer : public ReportRDD {
public:

  TermInfoMergeReducer(
      RefPtr<TermInfoTableSource> input,
      RefPtr<TermInfoTableSink> output);

  void onInit();
  void onTermInfo(const String& key, const TermInfo& t);
  void onFinish();

protected:
  RefPtr<TermInfoTableSource> input_table_;
  RefPtr<TermInfoTableSink> output_table_;
  HashMap<String, TermInfo> counters_;
};

} // namespace zbase

#endif
