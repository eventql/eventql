/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_JOINEDQUERYTABLEREPORT_H
#define _CM_JOINEDQUERYTABLEREPORT_H
#include "Report.h"

using namespace fnord;

namespace cm {

class JoinedQueryTableReport : public Report {
public:

  void addReport(RefPtr<Report> report);

  Set<String> inputFiles() override;
  Set<String> outputFiles() override;

protected:
  Set<String> input_files_;
  Set<String> output_files_;
};

} // namespace cm

#endif
