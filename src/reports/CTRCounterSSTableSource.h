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
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "CTRCounter.h"

using namespace fnord;

namespace cm {

class CTRCounterSSTableSource : public Report {
public:

  CTRCounterSSTableSource(const Set<String>& sstable_filenames);

  void onEvent(
      ReportEventType type,
      ReportEventTime time,
      void* ev) override;

  Set<String> inputFiles() override;

protected:

  void readTables();

  Set<String> input_files_;
  sstable::SSTableColumnSchema sstable_schema_;
};

} // namespace cm

#endif
