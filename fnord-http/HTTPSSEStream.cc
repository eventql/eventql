/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "HTTPSSEStream.h"

namespace fnord {
namespace http {

HTTPSSEStream::HTTPSSEStream(
  RefPtr<http::HTTPRequestStream> req_stream,
  RefPtr<http::HTTPResponseStream> res_stream) :
  req_stream_(req_stream),
  res_stream_(res_stream) {}

void HTTPSSEStream::start() {
  res_->setStatus(kStatusOK);
  res_->addHeader("Content-Type", "text/event-stream");
  res_->addHeader("Cache-Control", "no-cache");
  res_stream_->startResponse(*res_);
}


void HTTPSSEStream::sendEvent(const String& data) {
  Buffer chunk;
  chunk.append("data: ");
  chunk.append(data);
  chunk.append("\n\n");
  res_stream_->writeBodyChunk(chunk);
}

void HTTPSSEStream::sendEvent(
  const String& data,
  const String& id) {
  Buffer chunk;
  chunk.append("id: ");
  chunk.append(id);
  chunk.append("\ndata: ");
  chunk.append(data);
  chunk.append("\n\n");
  res_stream_->writeBodyChunk(chunk);
}

void HTTPSSEStream::finish() {
  res_stream_->finishResponse();
}


}
}
