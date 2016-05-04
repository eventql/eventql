/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/exception.h"
#include "stx/uri.h"
#include "stx/autoref.h"
#include "eventql/dproc/TaskSpec.pb.h"

using namespace stx;

namespace zbase {

class ReportFactory : public RefCounted {
public:
  typedef Function<dproc::TaskSpec (const URI::ParamList&)> FactoryFn;

  dproc::TaskSpec getReport(
      const String& name,
      const URI::ParamList& params) const;

  void registerReportFactory(const String& name, FactoryFn fn);

protected:
  HashMap<String, FactoryFn> reports_;
};

} // namespace zbase
