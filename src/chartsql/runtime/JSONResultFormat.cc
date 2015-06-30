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

  for (int i = 0; i < query->numStatements(); ++i) {
    json_->beginObject();

    //json_->addObjectEntry("columns");
    //json_->beginArray();

    //auto columns = query->getResultColumns(i);
    //for (int i = 0; i < columns.size(); ++i) {
    //  if (i > 0) {
    //    json_->addComma();
    //  }
    //  json_->addString(columns[i]);
    //}
    //json_->endArray();
    //json_->addComma();

    json_->addObjectEntry("rows");
    json_->beginArray();

    size_t j = 0;
    query->executeStatement(
        context,
        i,
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
  }

  json_->endArray();
  json_->endObject();
}

}
