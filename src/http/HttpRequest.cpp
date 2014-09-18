#include <xzero/http/HttpRequest.h>

namespace xzero {

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
      headers_(headers),
      input_(std::move(input)) {
  // .
}

void HttpRequest::recycle() {
  method_.clear();
  path_.clear();
  version_ = HttpVersion::UNKNOWN;
  secure_ = false;
  headers_.reset();
  input_ = nullptr;
}

}  // namespace xzero
