/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/defaultruntime.h>
#include "chartsql/defaults.h"

namespace csql {

DefaultRuntime::DefaultRuntime() : tables_(new TableRepository()) {
  installDefaultSymbols(&runtime_);
}

void DefaultRuntime::executeQuery(
    const String& query,
    RefPtr<csql::ResultFormat> result_format) {
  auto qplan = runtime_.parseAndBuildQueryPlan(
      query,
      tables_.get(),
      [] (RefPtr<QueryTreeNode> query) { return query; });

  runtime_.executeQuery(qplan, result_format);
}

void DefaultRuntime::addTableProvider(RefPtr<TableProvider> table) {
  tables_->addProvider(table);
}

}
