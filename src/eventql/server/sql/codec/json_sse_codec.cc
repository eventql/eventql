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
#include <eventql/server/sql/codec/json_sse_codec.h>

namespace eventql {

JSONSSECodec::JSONSSECodec(
    RefPtr<http::HTTPSSEStream> output) :
    output_(output) {}

void JSONSSECodec::sendResults(const Vector<csql::ResultList>& results) {
  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  json.beginObject();
  json.addObjectEntry("results");
  json.beginArray();

  for (size_t k = 0; k < results.size(); ++k) {
    if (k > 0) {
      json.addComma();
    }

    json.beginObject();
    json.addObjectEntry("type");
    json.addString("table");
    json.addComma();

    json.addObjectEntry("columns");
    json.beginArray();

    auto columns = results[k].getColumns();
    for (int i = 0; i < columns.size(); ++i) {
      if (i > 0) {
        json.addComma();
      }
      json.addString(columns[i]);
    }
    json.endArray();
    json.addComma();

    json.addObjectEntry("rows");
    json.beginArray();

    for (size_t j = 0; j < results[k].getNumRows(); ++j) {
      if (j > 0) {
        json.addComma();
      }

      json.beginArray();
      const auto& row = results[k].getRow(j);
      for (size_t i = 0; i < row.size(); ++i) {
        if (i > 0) {
          json.addComma();
        }

        json.addString(row[i]);
      }
      json.endArray();
    }

    json.endArray();
    json.endObject();
  }
  json.endArray();
  json.endObject();

  output_->sendEvent(buf, Some(String("result")));
}

void JSONSSECodec::sendProgress(
    bool done,
    double progress,
    size_t tasks_total,
    size_t tasks_complete,
    size_t tasks_running) {
  if (output_->isClosed()) {
    return;
  }

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("status");
  json.addString("running");
  json.addComma();
  json.addObjectEntry("progress");
  json.addFloat(progress);
  json.addComma();
  json.addObjectEntry("tasks_total");
  json.addInteger(tasks_total);
  json.addComma();
  json.addObjectEntry("tasks_complete");
  json.addInteger(tasks_complete);
  json.addComma();
  json.addObjectEntry("tasks_running");
  json.addInteger(tasks_running);
  json.addComma();
  json.addObjectEntry("message");
  if (progress == 0.0f) {
    json.addString("Waiting...");
  } else if (done) {
    json.addString("Downloading...");
  } else {
    json.addString("Running...");
  }
  json.endObject();

  output_->sendEvent(buf, Some(String("status")));
}

}
