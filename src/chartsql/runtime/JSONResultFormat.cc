/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/JSONResultFormat.h>
#include <fnord/charts/SVGTarget.h>

namespace csql {

JSONResultFormat::JSONResultFormat(
    json::JSONOutputStream* output) :
    json_(output) {}

void JSONResultFormat::formatResults(
    RefPtr<QueryPlan> query,
    ExecutionContext* context) {
  json_->beginObject();

  json_->addObjectEntry("result_tables");
  json_->beginArray();

  Vector<ScopedPtr<TableExpression>> statements;
  for (int i = 0; i < query->numStatements(); ++i) {
    auto stmt = query->buildStatement(i);

    json_->beginObject();

    json_->addObjectEntry("columns");
    json_->beginArray();

    auto columns = stmt->columnNames();
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
    stmt->execute(
        context,
        [this, &j] (int argc, const csql::SValue* argv) -> bool {
      if (++j > 1) {
        json_->addComma();
      }

      json_->beginArray();

      for (int n = 0; n < argc; ++n) {
        if (n > 0) {
          json_->addComma();
        }

        json_->addString(argv[n].toString());
      }

      json_->endArray();
      return true;
    });

    json_->endArray();
    json_->endObject();

    statements.emplace_back(std::move(stmt));
  }
  json_->endArray();
  json_->addComma();

  json_->addObjectEntry("result_charts");
  json_->beginArray();

  size_t nchart = 0;
  for (auto& stmt : statements) {
    auto draw_stmt = dynamic_cast<DrawStatement*>(stmt.get());
    if (!draw_stmt) {
      continue;
    }

    String svg_str;
    auto svg_stream = StringOutputStream::fromString(&svg_str);
    fnord::chart::SVGTarget svg(svg_stream.get());
    draw_stmt->render(&svg);

    if (++nchart > 1) json_->addComma();
    json_->addString(svg_str);
  }

  json_->endArray();
  json_->endObject();
}

}
