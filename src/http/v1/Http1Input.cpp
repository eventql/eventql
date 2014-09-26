#include <xzero/http/v1/Http1Input.h>
#include <xzero/http/HttpInputListener.h>
#include <xzero/logging/LogSource.h>
#include <xzero/sysconfig.h>

namespace xzero {
namespace http1 {

// TODO support buffering input into a temp file (via O_TMPFILE if available)

static LogSource inputLogger("http1.HttpInput");
#ifndef NDEBUG
#define TRACE(msg...) do { inputLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

Http1Input::Http1Input(HttpConnection* connection)
    : xzero::HttpInput(),
      connection_(connection),
      content_(),
      offset_(0) {
  TRACE("%p ctor", this);
}

Http1Input::~Http1Input() {
  TRACE("%p dtor", this);
}

void Http1Input::recycle() {
  TRACE("%p recycle", this);
  content_.clear();
  offset_ = 0;
}

int Http1Input::read(Buffer* result) {
  const size_t len = content_.size() - offset_;
  result->push_back(content_.ref(offset_));
  TRACE("%p read: %zu bytes", this, len);

  content_.clear();
  offset_ = 0;

  return len;
}

size_t Http1Input::readLine(Buffer* result) {
  const size_t len = content_.size() - offset_;
  TRACE("%p readLine: %zu bytes", this, len);

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

void Http1Input::onContent(const BufferRef& chunk) {
  TRACE("%p onContent: %zu bytes", this, chunk.size());
  content_ += chunk;

  if (listener())
    listener()->onContentAvailable();
}

} // namespace http1
} // namespace xzero
