#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpChannel.h>

namespace xzero {

HttpResponse::HttpResponse(HttpChannel* channel,
                           std::unique_ptr<HttpOutput>&& output)
    : channel_(channel),
      output_(std::move(output)),
      version_(HttpVersion::UNKNOWN),
      status_(HttpStatus::Undefined),
      contentType_(),
      contentLength_(static_cast<size_t>(-1)),
      headers_(),
      committed_(false) {
  //.
}

void HttpResponse::recycle() {
  output_->recycle();
  version_ = HttpVersion::UNKNOWN;
  status_ = HttpStatus::Undefined;
  reason_.clear();
  contentType_.clear();
  contentLength_ = static_cast<size_t>(-1);
  headers_.reset();
  committed_ = false;
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

void HttpResponse::setContentType(const std::string& value) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  //TODO
}

void HttpResponse::setContentLength(size_t size) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  contentLength_ = size;
}

void HttpResponse::addHeader(const std::string& name,
                             const std::string& value) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.push_back(name, value);
}

void HttpResponse::setHeader(const std::string& name,
                             const std::string& value) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.overwrite(name, value);
}

void HttpResponse::removeHeader(const std::string& name) {
  if (isCommitted())
    throw std::runtime_error("Invalid State. Cannot be modified after commit.");

  headers_.remove(name);
}

const std::string& HttpResponse::getHeader(const std::string& name) const {
  return headers_.get(name);
}

void HttpResponse::send100Continue() {
  channel_->send100Continue();
}

void HttpResponse::completed() {
  output_->completed();
}

}  // namespace xzero
