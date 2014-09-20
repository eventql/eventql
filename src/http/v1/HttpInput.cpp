#include <xzero/http/v1/HttpInput.h>
#include <xzero/http/HttpInputListener.h>
#include <xzero/logging/LogSource.h>

namespace xzero {
namespace http1 {

static LogSource inputLogger("http1.HttpInput");
#ifndef NDEBUG
#define TRACE(msg...) do { inputLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

HttpInput::HttpInput(HttpConnection* connection)
    : xzero::HttpInput(),
      connection_(connection),
      content_(),
      offset_(0) {
  TRACE("%p ctor", this);
}

HttpInput::~HttpInput() {
  TRACE("%p dtor", this);
}

void HttpInput::recycle() {
  TRACE("%p recycle", this);
  content_.clear();
  offset_ = 0;
}

int HttpInput::read(Buffer* result) {
  const size_t len = content_.size() - offset_;
  result->push_back(content_.ref(offset_));
  TRACE("%p read: %zu bytes\n", this, len);

  content_.clear();
  offset_ = 0;

  return len;
}

size_t HttpInput::readLine(Buffer* result) {
  const size_t len = content_.size() - offset_;
  TRACE("%p readLine: %zu bytes\n", this, len);

  const size_t n = content_.find('\n', offset_);
  if (n == Buffer::npos) {
    result->push_back(content_);
    content_.clear();
    offset_ = 0;
    return len;
  }

  result->push_back(content_.ref(offset_, n - offset_));
  offset_ = n + 1;

  if (offset_ == content_.size()) {
    content_.clear();
    offset_ = 0;
  }

  return 0;
}

void HttpInput::onContent(const BufferRef& chunk) {
  if (!content_.empty())
    throw std::runtime_error("Illegal State. HttpInput received data but still has (unhandled) data.");

  content_ += chunk;

  if (listener())
    listener()->onContentAvailable();
}

} // namespace http1
} // namespace xzero
