/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/sql/codec/json_codec.h>
#include <eventql/sql/runtime/resultlist.h>

namespace zbase {

void JSONCodec::formatResultList(
    const csql::ResultList* result,
    json::JSONOutputStream* json) {
  json->beginObject();
  json->addObjectEntry("type");
  json->addString("table");
  json->addComma();

  json->addObjectEntry("columns");
  json->beginArray();

  auto columns = result->getColumns();
  for (int n = 0; n < columns.size(); ++n) {
    if (n > 0) {
      json->addComma();
    }
    json->addString(columns[n]);
  }
  json->endArray();
  json->addComma();

  json->addObjectEntry("rows");
  json->beginArray();

  size_t j = 0;
  size_t m = columns.size();
  for (; j < result->getNumRows(); ++j) {
    const auto& row = result->getRow(j);

    if (++j > 1) {
      json->addComma();
    }

    json->beginArray();

    size_t n = 0;
    for (; n < m && n < row.size(); ++n) {
      if (n > 0) {
        json->addComma();
      }

      json->addString(row[n]);
    }

    for (; n < m; ++n) {
      if (n > 0) {
        json->addComma();
      }

      json->addNull();
    }

    json->endArray();
  }

  json->endArray();
  json->endObject();
}

JSONCodec::JSONCodec(csql::QueryPlan* query) {
  for (size_t i = 0; i < query->numStatements(); ++i) {
    auto result = mkScoped(new csql::ResultList());
    query->storeResults(i, result.get());
    results_.emplace_back(std::move(result));
  }
}

void JSONCodec::printResults(ScopedPtr<OutputStream> os) {
  json::JSONOutputStream json(std::move(os));

  json.beginObject();

  json.addObjectEntry("results");
  json.beginArray();

  for (int i = 0; i < results_.size(); ++i) {
    formatResultList(results_[i].get(), &json);

    if (i > 0) {
      json.addComma();
    }
  }

  json.endArray();
  json.endObject();
}

}
