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
#include "zbase/Report.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"
#include "TermInfo.h"

using namespace stx;

namespace zbase {

class TermInfoTableSource : public ReportSource {
public:
  typedef Function<void (const String&, const TermInfo&)> CallbackFn;

  TermInfoTableSource(const List<dproc::TaskDependency>& deps);

  void forEach(CallbackFn fn);

  void read(dproc::TaskContext* context);
  List<dproc::TaskDependency> dependencies() const override;

protected:
  List<dproc::TaskDependency> deps_;
  sstable::SSTableColumnSchema schema_;
  List<CallbackFn> callbacks_;
};

} // namespace zbase

#endif
