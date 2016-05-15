/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/sql/codec/json_sse_codec.h>
#include <eventql/sql/runtime/resultlist.h>

namespace eventql {

JSONSSECodec::JSONSSECodec(
    csql::QueryPlan* query,
    RefPtr<http::HTTPSSEStream> output) :
    json_codec_(query),
    output_(output) {
  query->onQueryFinished(std::bind(&JSONSSECodec::sendResults, this));
}

void JSONSSECodec::sendResults() {
  Buffer result;
  json_codec_.printResults(BufferOutputStream::fromBuffer(&result));
  output_->sendEvent(result, Some(String("result")));
}

void JSONSSECodec::sendProgress(double progress) {
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
  json.addObjectEntry("message");
  if (progress == 0.0f) {
    json.addString("Waiting...");
  } else if (progress == 1.0f) {
    json.addString("Downloading...");
  } else {
    json.addString("Running...");
  }
  json.endObject();

  output_->sendEvent(buf, Some(String("status")));
}

}
