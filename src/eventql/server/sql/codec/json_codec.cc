/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/server/sql/codec/json_codec.h>

namespace eventql {

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

  Vector<csql::SValue> row;
  for (size_t i = 0; i < cursor->getColumnCount(); ++i) {
    row.emplace_back(cursor->getColumnType(i));
  }

  for (size_t i = 0; cursor->next(row.data()); ++i) {
    if (i > 0) {
      json_->addComma();
    }

    json_->beginArray();
    auto n_max = std::min(row.size(), header.size());
    for (size_t n = 0; n < n_max; ++n) {
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
