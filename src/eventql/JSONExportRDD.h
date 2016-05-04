/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/wallclock.h>
#include <eventql/dproc/Task.h>
#include <eventql/dproc/BlobRDD.h>
#include "eventql/VTable.h"

using namespace stx;

namespace zbase {

class JSONExportRDD : public dproc::BlobRDD {
public:

  JSONExportRDD(RefPtr<VTableSource> source);

  RefPtr<VFSFile> computeBlob(dproc::TaskContext* context) override;

  List<dproc::TaskDependency> dependencies() const override;

  String contentType() const override;

protected:
  RefPtr<VTableSource> source_;
};

} // namespace zbase

