/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRCOUNTERSSTABLESINK_H
#define _CM_CTRCOUNTERSSTABLESINK_H
#include "eventql/Report.h"
#include "eventql/CTRCounter.h"
#include <eventql/docdb/ItemRef.h>

#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"

using namespace stx;

namespace zbase {

class CTRCounterTableSink : public ReportSink {
public:

  CTRCounterTableSink(
      const String& tempdir = "/tmp"); // FIXPAUL!

  void open();
  void addRow(const String& key, CTRCounterData counter);
  RefPtr<VFSFile> finalize();

  String contentType() const {
    return "application/octet-stream";
  }

protected:
  String output_file_;
  std::unique_ptr<sstable::SSTableEditor> sstable_writer_;
  sstable::SSTableColumnSchema sstable_schema_;
};

} // namespace zbase

#endif
