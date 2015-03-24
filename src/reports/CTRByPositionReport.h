/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYPOSITIONREPORT_H
#define _CM_CTRBYPOSITIONREPORT_H
#include "reports/Report.h"
#include "JoinedQuery.h"

using namespace fnord;

namespace cm {

class CTRByPositionReport : public Report {
public:

  CTRByPositionReport(const String& output_file);

  void onEvent(ReportEventType type, void* ev) override;

  Set<String> inputFiles() override;
  Set<String> outputFiles() override;

protected:
  String output_file_;
};

} // namespace cm

#endif
