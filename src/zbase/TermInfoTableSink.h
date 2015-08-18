/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TERMINFOTABLESINK_H
#define _CM_TERMINFOTABLESINK_H
#include "zbase/Report.h"

#include <inventory/ItemRef.h>
#include "zbase/TermInfo.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"

using namespace stx;

namespace zbase {

class TermInfoTableSink : public ReportSink {
public:

  TermInfoTableSink(const String& tempdir = "/tmp");

  void open();
  void addRow(const String& key, const TermInfo& term_info);
  RefPtr<VFSFile> finalize();

  String contentType() const override {
    return "application/octet-stream";
  }

protected:
  String output_file_;
  std::unique_ptr<sstable::SSTableEditor> sstable_writer_;
  sstable::SSTableColumnSchema sstable_schema_;
};

} // namespace zbase

#endif
