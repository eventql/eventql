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
#include "eventql/Report.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/sstable/SSTableEditor.h"
#include "eventql/infra/sstable/SSTableColumnSchema.h"
#include "eventql/infra/sstable/SSTableColumnReader.h"
#include "eventql/infra/sstable/SSTableColumnWriter.h"
#include "eventql/CTRCounter.h"

using namespace stx;

namespace zbase {

class CTRCounterTableSource : public ReportSource {
public:
  typedef Function<void (const String&, const CTRCounterData&)> CallbackFn;

  CTRCounterTableSource(const List<dproc::TaskDependency>& deps);

  void forEach(CallbackFn fn);

  void read(dproc::TaskContext* context);
  List<dproc::TaskDependency> dependencies() const override;

protected:
  List<dproc::TaskDependency> deps_;
  sstable::SSTableColumnSchema sstable_schema_;
  List<CallbackFn> callbacks_;
};

} // namespace zbase

#endif
