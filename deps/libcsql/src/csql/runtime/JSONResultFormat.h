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
#include <csql/runtime/queryplan.h>
#include <csql/runtime/charts/ChartStatement.h>
#include <csql/runtime/ResultFormat.h>

namespace csql {

class JSONResultFormat : public ResultFormat {
public:

  JSONResultFormat(json::JSONOutputStream* json);

  void formatResults(
      RefPtr<QueryPlan> query,
      ExecutionContext* context);

protected:

  void renderStatement(
      RefPtr<QueryTreeNode> qtree,
      Statement* stmt,
      ExecutionContext* context);

  void renderTable(
      RefPtr<QueryTreeNode> qtree,
      TableExpression* stmt,
      ExecutionContext* context);

  void renderChart(
      ChartStatement* stmt,
      ExecutionContext* context);

  json::JSONOutputStream* json_;
};

}
