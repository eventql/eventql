/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "zbase/Report.h"
#include "zbase/TermInfoTableSource.h"
#include "zbase/CSVSink.h"
#include "zbase/TermInfo.h"
#include <inventory/ItemRef.h>

using namespace stx;

namespace zbase {

class TopCategoriesByTermReport : public ReportRDD {
public:

  TopCategoriesByTermReport(
      RefPtr<TermInfoTableSource> input,
      RefPtr<CSVSink> output);

  void onInit();
  void onTermInfo(const String& term, const TermInfo& ti);
  void onFinish();

protected:
  RefPtr<TermInfoTableSource> input_table_;
  RefPtr<CSVSink> output_table_;
};

} // namespace zbase
