// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpOutputCompressor.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/io/Filter.h>
#include <xzero/io/GzipFilter.h>
#include <xzero/Tokenizer.h>
#include <xzero/Buffer.h>
#include <algorithm>
#include <system_error>
#include <stdexcept>
#include <cstdlib>

#include <xzero/sysconfig.h>

namespace xzero {

HttpOutputCompressor::HttpOutputCompressor()
    : contentTypes_(),              // no types
      level_(9),                    // best compression
      minSize_(256),                // 256 byte
      maxSize_(128 * 1024 * 1024) { // 128 MB
  addMimeType("text/plain");
  addMimeType("text/html");
  addMimeType("text/css");
  addMimeType("application/xml");
  addMimeType("application/xhtml+xml");
  addMimeType("application/javascript");
}

HttpOutputCompressor::~HttpOutputCompressor() {
}

void HttpOutputCompressor::setMinSize(size_t value) {
  minSize_ = value;
}

void HttpOutputCompressor::setMaxSize(size_t value) {
  maxSize_ = value;
}

void HttpOutputCompressor::addMimeType(const std::string& value) {
  contentTypes_[value] = 0;
}

bool HttpOutputCompressor::containsMimeType(const std::string& value) const {
  return contentTypes_.find(value) != contentTypes_.end();
}

template<typename Encoder>
bool tryEncode(const std::string& encoding,
               int level,
               const std::vector<BufferRef>& accepts,
               HttpRequest* request,
               HttpResponse* response) {
  if (std::find(accepts.begin(), accepts.end(), encoding) == accepts.end())
    return false;

  // response might change according to Accept-Encoding
  response->appendHeader("Vary", "Accept-Encoding", ",");

  // removing content-length implicitely enables chunked encoding
  response->resetContentLength();

  response->addHeader("Content-Encoding", encoding);
  response->output()->addFilter(std::make_shared<Encoder>(level));

  return true;
}

void HttpOutputCompressor::inject(HttpRequest* request,
                                  HttpResponse* response) {
  // TODO: ensure postProcess() gets invoked right before response commit
}

void HttpOutputCompressor::postProcess(HttpRequest* request,
                                       HttpResponse* response) {
  if (response->headers().contains("Content-Encoding"))
    return;  // do not double-encode content

  bool chunked = !response->hasContentLength();
  long long size = chunked ? 0 : response->contentLength();

  if (!chunked && (size < minSize_ || size > maxSize_))
    return;

  if (!containsMimeType(response->headers().get("Content-Type")))
    return;

  const std::string& acceptEncoding = request->headers().get("Accept-Encoding");
  BufferRef r(acceptEncoding);
  if (!r.empty()) {
    const auto items = Tokenizer<BufferRef, BufferRef>::tokenize(r, ", ");

    // if (tryEncode<Bzip2Filter>("bzip2", level_, items, request, response))
    //   return;

    if (tryEncode<GzipFilter>("gzip", level_, items, request, response))
      return;
  }
}

} // namespace xzero
