#include <xzero/http/HttpInput.h>

namespace xzero {

HttpInput::HttpInput()
    : listener_(nullptr) {
}

HttpInput::~HttpInput() {
}

void HttpInput::setListener(HttpInputListener* listener) {
  listener_ = listener;
}

} // namespace xzero
