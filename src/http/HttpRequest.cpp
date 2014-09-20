#include <xzero/http/HttpRequest.h>
#include <xzero/logging/LogSource.h>

namespace xzero {

static LogSource requestLogger("http1.HttpRequest");
#ifndef NDEBUG
#define TRACE(msg...) do { requestLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

HttpRequest::HttpRequest()
    : HttpRequest("", "", HttpVersion::UNKNOWN, false, {}, nullptr) {
  // .
}

HttpRequest::HttpRequest(std::unique_ptr<HttpInput>&& input)
    : HttpRequest("", "", HttpVersion::UNKNOWN, false, {}, std::move(input)) {
  // .
}

HttpRequest::HttpRequest(const std::string& method, const std::string& path,
                         HttpVersion version, bool secure,
                         const HeaderFieldList& headers,
                         std::unique_ptr<HttpInput>&& input)
    : method_(method),
      path_(path),
      version_(version),
      secure_(secure),
      expect100Continue_(false),
      headers_(headers),
      input_(std::move(input)) {
  // .
}

void HttpRequest::recycle() {
  TRACE("%p recycle", this);
  method_.clear();
  path_.clear();
  version_ = HttpVersion::UNKNOWN;
  secure_ = false;
  headers_.reset();
  input_->recycle();
}

}  // namespace xzero
