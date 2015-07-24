/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include <chartsql/runtime/queryplan.h>
#include <chartsql/runtime/charts/ChartStatement.h>

namespace csql {

class JSONResultFormat {
public:

  JSONResultFormat(json::JSONOutputStream* json);

  void formatResults(
      RefPtr<QueryPlan> query,
      ExecutionContext* context);

protected:

  void renderStatement(
      Statement* stmt,
      ExecutionContext* context);

  void renderTable(
      TableExpression* stmt,
      ExecutionContext* context);

  void renderChart(
      ChartStatement* stmt,
      ExecutionContext* context);

  json::JSONOutputStream* json_;
};

}
