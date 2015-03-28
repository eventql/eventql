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
#include "reports/Report.h"
#include "CTRCounterTableSource.h"
#include "TermInfoTableSink.h"
#include "TermInfoTableSource.h"
#include "TermInfo.h"
#include "ItemRef.h"
#include "common.h"

using namespace fnord;

namespace cm {

/**
 * INPUT: TERM_INFO (key=<lang>~<term>)
 * OUTPUT: TERM_INFO (key=<lang>~<term>)
 */
class TermInfoMergeReducer : public Report {
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

} // namespace cm

#endif
