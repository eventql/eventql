/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/ReportFactory.h"

using namespace stx;

namespace zbase {

dproc::TaskSpec ReportFactory::getReport(
    const String& name,
    const URI::ParamList& params) const {
  auto iter = reports_.find(name);
  if (iter == reports_.end()) {
    RAISEF(kIndexError, "report not found: '$0'", name);
  }

  return iter->second(params);
}

void ReportFactory::registerReportFactory(const String& name, FactoryFn fn) {
  reports_.emplace(name, fn);
}


} // namespace zbase
