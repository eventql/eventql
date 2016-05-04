/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/sql/codec/json_sse_codec.h>
#include <eventql/sql/runtime/resultlist.h>

namespace zbase {

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
