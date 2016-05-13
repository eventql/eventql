/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/server/sql/codec/json_codec.h>
#include <eventql/sql/runtime/resultlist.h>

namespace zbase {

JSONCodec::JSONCodec(json::JSONOutputStream* json) : json_(json) {}

void JSONCodec::printResultTable(
    const Vector<String>& header,
    csql::ResultCursor* cursor) {
  json_->beginObject();
  json_->addObjectEntry("type");
  json_->addString("table");
  json_->addComma();

  json_->addObjectEntry("columns");
  json_->beginArray();

  for (int n = 0; n < header.size(); ++n) {
    if (n > 0) {
      json_->addComma();
    }
    json_->addString(header[n]);
  }
  json_->endArray();
  json_->addComma();

  json_->addObjectEntry("rows");
  json_->beginArray();

  Vector<csql::SValue> row(cursor->getNumColumns());
  for (size_t i = 0; cursor->next(row.data(), row.size()); ++i) {
    if (i > 0) {
      json_->addComma();
    }

    json_->beginArray();
    for (size_t n = 0; n < row.size(); ++n) {
      if (n > 0) {
        json_->addComma();
      }

      json_->addString(row[n].getString());
    }
    json_->endArray();
  }

  json_->endArray();
  json_->endObject();
}

}
