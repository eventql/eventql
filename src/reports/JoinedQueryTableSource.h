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
  typedef Function<void (const JoinedQuery& query)> CallbackFn;

  JoinedQueryTableSource(const String& sstable_filename);
  JoinedQueryTableSource(const Set<String>& sstable_filenames);

  void forEach(CallbackFn fn);

  void read() override;
  Set<String> inputFiles() override;

protected:
  void readTables();
  Set<String> input_files_;
  List<CallbackFn> callbacks_;
};


} // namespace cm

#endif
