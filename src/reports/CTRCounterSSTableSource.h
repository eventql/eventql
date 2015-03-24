/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRCONTERSSTABLESOURCE_H
#define _CM_CTRCONTERSSTABLESOURCE_H
#include "reports/Report.h"
#include "CTRCounterSSTableSource.h"

using namespace fnord;

namespace cm {

class CTRCounterSSTableSource : public Report {
public:

  CTRCounterSSTableSource(const Set<String>& sstable_filenames);

  void onEvent(ReportEventType type, void* ev) override;

  Set<String> inputFiles() override;

protected:

  void readTables();

  Set<String> input_files_;
};

} // namespace cm

#endif
