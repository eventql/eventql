/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/JSONSSEStreamFormat.h>
#include <csql/runtime/JSONResultFormat.h>
#include <stx/logging.h>

namespace csql {

JSONSSEStreamFormat::JSONSSEStreamFormat(
    http::HTTPSSEStream* output) :
    output_(output) {}

void JSONSSEStreamFormat::formatResults(
    RefPtr<QueryPlan> query,
    ExecutionContext* context) {
  try {
    context->onStatusChange([this, context] (const csql::ExecutionStatus& status) {
      auto progress = status.progress();

      if (output_->isClosed()) {
        stx::logDebug("sql", "Aborting Query...");
        context->cancel();
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
    });

    Buffer result;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&result));
    JSONResultFormat format(&json);
    format.formatResults(query, context);
    output_->sendEvent(result, Some(String("result")));
  } catch (const StandardException& e) {
    stx::logError("sql", e, "SQL execution failed");

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    output_->sendEvent(buf, Some(String("error")));
  }
}

}
