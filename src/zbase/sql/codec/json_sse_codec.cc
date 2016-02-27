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
#include <zbase/sql/codec/json_sse_codec.h>
#include <csql/runtime/resultlist.h>

namespace zbase {

JSONSSECodec::JSONSSECodec(
    csql::QueryPlan* query,
    RefPtr<http::HTTPSSEStream> output) :
    output_(output) {
  for (size_t i = 0; i < query->numStatements(); ++i) {
    auto result = mkScoped(new csql::ResultList());
    query->storeResults(i, result.get());
    results_.emplace_back(std::move(result));
    query->onOutputComplete(i, std::bind(&JSONSSECodec::sendResult, this, i));
  }
}

void JSONSSECodec::sendResult(size_t idx) {
  Buffer result;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&result));
  json.beginObject();
  json.addObjectEntry("results");
  json.beginArray();
  JSONCodec::formatResultList(results_[idx].get(), &json);
  json.endArray();
  json.endObject();
  output_->sendEvent(result, Some(String("result")));
}

}
