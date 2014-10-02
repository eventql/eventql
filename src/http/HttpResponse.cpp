#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpChannel.h>
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
  output_->recycle();
}

void HttpResponse::setCommitted(bool value) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  committed_ = value;
}

HttpVersion HttpResponse::version() const noexcept {
  return version_;
}

void HttpResponse::setVersion(HttpVersion version) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  version_ = version;
}

void HttpResponse::setStatus(HttpStatus status) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  status_ = status;
}

void HttpResponse::setReason(const std::string& val) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  reason_ = val;
}

void HttpResponse::setContentLength(size_t size) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  contentLength_ = size;
}

static const std::vector<std::string> connectionHeaderFields = {
  "Connection",
  "Keep-Alive",
  "Transfer-Encoding"
  "Close",
};

inline void checkInvalidHeader(const std::string& name) {
  for (const auto& test: connectionHeaderFields)
    if (iequals(name, test))
      throw std::runtime_error("Invalid argument. Harmful response header.");
}

void HttpResponse::addHeader(const std::string& name,
                             const std::string& value) {
  checkInvalidHeader(name);

  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.push_back(name, value);
}

void HttpResponse::appendHeader(const std::string& name,
                                const std::string& value,
                                const std::string& delim) {
  checkInvalidHeader(name);

  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.append(name, value, delim);
}

void HttpResponse::setHeader(const std::string& name,
                             const std::string& value) {
  checkInvalidHeader(name);

  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.overwrite(name, value);
}

void HttpResponse::removeHeader(const std::string& name) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.remove(name);
}

void HttpResponse::removeAllHeaders() {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.reset();
}

const std::string& HttpResponse::getHeader(const std::string& name) const {
  return headers_.get(name);
}

void HttpResponse::send100Continue() {
  channel_->send100Continue();
}

void HttpResponse::sendError(HttpStatus code, const std::string& message) {
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
    output()->write(std::move(body));
  }

  completed();
}

void HttpResponse::completed() {
  output_->completed();
}

}  // namespace xzero
