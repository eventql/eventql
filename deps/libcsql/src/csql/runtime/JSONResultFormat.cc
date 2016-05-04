/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/JSONResultFormat.h>
#include <cplot/svgtarget.h>

namespace csql {

JSONResultFormat::JSONResultFormat(
    json::JSONOutputStream* output) :
    json_(output) {}

void JSONResultFormat::formatResults(
    ScopedPtr<QueryPlan> query,
    ExecutionContext* context) {
  json_->beginObject();

  json_->addObjectEntry("results");
  json_->beginArray();

  for (int i = 0; i < query->numStatements(); ++i) {
    auto qtree = query->getStatementQTree(i);
    auto stmt = query->getStatement(i);

    if (i > 0) {
      json_->addComma();
    }

    renderStatement(qtree, stmt, context);
  }

  json_->endArray();
  json_->endObject();
}

void JSONResultFormat::renderStatement(
    RefPtr<QueryTreeNode> qtree,
    Statement* stmt,
    ExecutionContext* context) {
  auto table_expr = dynamic_cast<TableExpression*>(stmt);
  if (table_expr) {
    renderTable(qtree, table_expr, context);
    return;
  }

  auto chart_expr = dynamic_cast<ChartStatement*>(stmt);
  if (chart_expr) {
    renderChart(chart_expr, context);
    return;
  }

  RAISE(kRuntimeError, "can't render statement in JSONResultFormat");
}

void JSONResultFormat::renderTable(
    RefPtr<QueryTreeNode> qtree,
    TableExpression* stmt,
    ExecutionContext* context) {
  json_->beginObject();
  json_->addObjectEntry("type");
  json_->addString("table");
  json_->addComma();

  json_->addObjectEntry("columns");
  json_->beginArray();

  auto columns = qtree.asInstanceOf<TableExpressionNode>()->outputColumns();
  for (int n = 0; n < columns.size(); ++n) {
    if (n > 0) {
      json_->addComma();
    }
    json_->addString(columns[n]);
  }
  json_->endArray();
  json_->addComma();

  json_->addObjectEntry("rows");
  json_->beginArray();

  size_t j = 0;
  size_t m = columns.size();
  stmt->execute(
      context,
      [this, &j, m] (int argc, const csql::SValue* argv) -> bool {
    if (++j > 1) {
      json_->addComma();
    }

    json_->beginArray();

    size_t n = 0;
    for (; n < m && n < argc; ++n) {
      if (n > 0) {
        json_->addComma();
      }

      json_->addString(argv[n].getString());
    }

    for (; n < m; ++n) {
      if (n > 0) {
        json_->addComma();
      }

      json_->addNull();
    }

    json_->endArray();
    return true;
  });

  json_->endArray();
  json_->endObject();
}

void JSONResultFormat::renderChart(
    ChartStatement* stmt,
    ExecutionContext* context) {
  String svg_str;
  auto svg_stream = StringOutputStream::fromString(&svg_str);
  stx::chart::SVGTarget svg(svg_stream.get());
  stmt->execute(context, &svg);

  json_->beginObject();
  json_->addObjectEntry("type");
  json_->addString("chart");
  json_->addComma();
  json_->addObjectEntry("svg");
  json_->addString(svg_str);
  json_->endObject();
}
}
