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
#include "reports/Report.h"
#include "JoinedQuery.h"

using namespace fnord;

namespace cm {

class JoinedQueryTableSource : public ReportSource {
public:

  JoinedQueryTableSource(const String& sstable_filename);
  JoinedQueryTableSource(const Set<String>& sstable_filenames);

  Set<String> inputFiles() override;

protected:
  void readTables();
  Set<String> input_files_;
};


} // namespace cm

#endif
