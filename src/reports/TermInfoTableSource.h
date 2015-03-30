/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TERMINFOTABLESOURCE_H
#define _CM_TERMINFOTABLESOURCE_H
#include "reports/Report.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "TermInfo.h"

using namespace fnord;

namespace cm {

class TermInfoTableSource : public ReportSource {
public:
  typedef Function<void (const String&, const TermInfo&)> CallbackFn;

  TermInfoTableSource(const Set<String>& sstable_filenames);

  void forEach(CallbackFn fn);

  void read();
  Set<String> inputFiles() override;

protected:
  Set<String> input_files_;
  sstable::SSTableColumnSchema schema_;
  List<CallbackFn> callbacks_;
};

} // namespace cm

#endif
