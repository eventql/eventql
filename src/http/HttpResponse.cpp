#include <xzero/http/HttpResponse.h>

namespace xzero {

HttpResponse::HttpResponse(std::unique_ptr<HttpOutput>&& output)
    : output_(std::move(output)),
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
  assert(isCommitted() == false && "Cannot be modified after commit.");
  committed_ = value;
}

HttpVersion HttpResponse::version() const noexcept {
  return version_;
}

void HttpResponse::setVersion(HttpVersion version) {
  assert(isCommitted() == false && "Cannot be modified after commit.");
  version_ = version;
}

void HttpResponse::setStatus(HttpStatus status) {
  assert(isCommitted() == false && "Cannot be modified after commit.");
  status_ = status;
}

void HttpResponse::setContentType(const std::string& value) {
  assert(isCommitted() == false && "Cannot be modified after commit.");
  //TODO
}

void HttpResponse::setContentLength(size_t size) {
  assert(isCommitted() == false && "Cannot be modified after commit.");
  contentLength_ = size;
}

void HttpResponse::addHeader(const std::string& name,
                             const std::string& value) {
  assert(isCommitted() == false && "Cannot be modified after commit.");
  headers_.push_back(name, value);
}

void HttpResponse::setHeader(const std::string& name,
                             const std::string& value) {
  assert(isCommitted() == false && "Cannot be modified after commit.");
  headers_.overwrite(name, value);
}

void HttpResponse::removeHeader(const std::string& name) {
  assert(isCommitted() == false && "Cannot be modified after commit.");
  headers_.remove(name);
}

const std::string& HttpResponse::getHeader(const std::string& name) const {
  return headers_.get(name);
}

void HttpResponse::completed() {
  output_->completed();
}

}  // namespace xzero
