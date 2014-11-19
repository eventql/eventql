// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpChannel.h>
#include <xzero/RuntimeError.h>
#include <vector>
#include <string>

namespace xzero {

HttpResponse::HttpResponse(HttpChannel* channel,
                           std::unique_ptr<HttpOutput>&& output)
    : channel_(channel),
      output_(std::move(output)),
      version_(HttpVersion::UNKNOWN),
      status_(HttpStatus::Undefined),
      contentLength_(static_cast<size_t>(-1)),
      headers_(),
      trailers_(),
      committed_(false) {
  //.
}

void HttpResponse::recycle() {
  committed_ = false;
  version_ = HttpVersion::UNKNOWN;
  status_ = HttpStatus::Undefined;
  reason_.clear();
  contentLength_ = static_cast<size_t>(-1);
  headers_.reset();
  trailers_.reset();
  output_->recycle();
}

void HttpResponse::checkState() {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");
}

void HttpResponse::checkChannelState() {
  switch (channel_->state()) {
    case HttpChannelState::READING:
    case HttpChannelState::HANDLING:
      break;
    case HttpChannelState::SENDING:
    default:
      throw RUNTIME_ERROR("Attempt to modify response while in wrong channel state.");
  }
}

void HttpResponse::setCommitted(bool value) {
  checkState();
  committed_ = value;
}

HttpVersion HttpResponse::version() const XZERO_NOEXCEPT {
  return version_;
}

void HttpResponse::setVersion(HttpVersion version) {
  checkState();
  checkChannelState();

  version_ = version;
}

void HttpResponse::setStatus(HttpStatus status) {
  checkState();
  checkChannelState();

  status_ = status;
}

void HttpResponse::setReason(const std::string& val) {
  checkState();
  checkChannelState();

  reason_ = val;
}

void HttpResponse::setContentLength(size_t size) {
  checkState();
  checkChannelState();

  contentLength_ = size;
}

void HttpResponse::resetContentLength() {
  checkState();
  checkChannelState();

  contentLength_ = static_cast<size_t>(-1);
}

static const std::vector<std::string> connectionHeaderFields = {
  "Connection",
  "Content-Length",
  "Close",
  "Keep-Alive",
  "TE",
  "Trailer",
  "Transfer-Encoding",
  "Upgrade",
  "Via",
};

inline void checkInvalidHeader(const std::string& name) {
  for (const auto& test: connectionHeaderFields)
    if (iequals(name, test))
      throw std::runtime_error("Invalid argument. Harmful response header.");
}

void HttpResponse::addHeader(const std::string& name,
                             const std::string& value) {
  checkState();
  checkChannelState();
  checkInvalidHeader(name);

  headers_.push_back(name, value);
}

void HttpResponse::appendHeader(const std::string& name,
                                const std::string& value,
                                const std::string& delim) {
  checkState();
  checkChannelState();
  checkInvalidHeader(name);

  headers_.append(name, value, delim);
}

void HttpResponse::setHeader(const std::string& name,
                             const std::string& value) {
  checkState();
  checkChannelState();
  checkInvalidHeader(name);

  headers_.overwrite(name, value);
}

void HttpResponse::removeHeader(const std::string& name) {
  checkState();
  checkChannelState();

  headers_.remove(name);
}

void HttpResponse::removeAllHeaders() {
  checkState();
  checkChannelState();

  headers_.reset();
}

const std::string& HttpResponse::getHeader(const std::string& name) const {
  return headers_.get(name);
}

void HttpResponse::send100Continue() {
  channel_->send100Continue();
}

void HttpResponse::sendError(HttpStatus code, const std::string& message) {
  checkState();
  checkChannelState();

  setStatus(code);
  setReason(message);
  removeAllHeaders();
  output()->removeAllFilters();

  if (!isContentForbidden(code)) {
    Buffer body(2048);

    Buffer htmlMessage = message.empty() ? to_string(code) : message;

    htmlMessage.replaceAll("<", "&lt;");
    htmlMessage.replaceAll(">", "&gt;");
    htmlMessage.replaceAll("&", "&amp;");

    body << "<DOCTYPE html>\n"
            "<html>\n"
            "  <head>\n"
            "    <title> Error. " << htmlMessage << " </title>\n"
            "  </head>\n"
            "  <body>\n"
            "    <h1> Error. " << htmlMessage << " </h1>\n"
            "  </body>\n"
            "</html>\n";

    setHeader("Cache-Control", "must-revalidate,no-cache,no-store");
    setHeader("Content-Type", "text/html");
    setContentLength(body.size());
    output()->write(std::move(body), std::bind(&HttpResponse::completed, this));
  } else {
    completed();
  }
}

// {{{ trailers
void HttpResponse::registerTrailer(const std::string& name) {
  checkState();
  checkChannelState();
  checkInvalidHeader(name);

  if (trailers_.contains(name))
    throw std::runtime_error("Trailer already registered.");

  trailers_.push_back(name, "");
}

void HttpResponse::appendTrailer(const std::string& name,
                                 const std::string& value,
                                 const std::string& delim) {
  checkChannelState();
  checkInvalidHeader(name);

  if (!trailers_.contains(name))
    throw std::runtime_error("Trailer not registered.");

  trailers_.append(name, value, delim);
}

void HttpResponse::setTrailer(const std::string& name, const std::string& value) {
  checkChannelState();
  checkInvalidHeader(name);

  if (!trailers_.contains(name))
    throw std::runtime_error("Trailer not registered.");

  trailers_.overwrite(name, value);
}
// }}}

void HttpResponse::completed() {
  output_->completed();
}

}  // namespace xzero
