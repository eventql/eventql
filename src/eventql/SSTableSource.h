/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_SSTABLETABLESOURCE_H
#define _CM_SSTABLETABLESOURCE_H
#include "stx/logging.h"
#include "eventql/Report.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"

using namespace stx;

namespace zbase {

template <typename T>
class SSTableSource : public ReportSource {
public:
  typedef Function<void (const String&, const T&)> CallbackFn;

  SSTableSource(const List<dproc::TaskDependency>& deps);

  void forEach(CallbackFn fn);

  void read(dproc::TaskContext* context);
  List<dproc::TaskDependency> dependencies() const override;

protected:
  List<dproc::TaskDependency> deps_;
  List<CallbackFn> callbacks_;
};

} // namespace zbase

#include "SSTableSource_impl.h"
#endif
