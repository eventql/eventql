#include <xzero/http/HttpListener.h>

namespace xzero {

bool HttpListener::onMessageBegin(const BufferRef& method, const BufferRef& uri,
                                  int versionMajor, int versionMinor) {
  return true;
}

bool HttpListener::onMessageBegin(int versionMajor, int versionMinor, int code,
                                  const BufferRef& text) {
  return true;
}

bool HttpListener::onMessageBegin() {
  //.
  return true;
}

bool HttpListener::onMessageHeader(const BufferRef& name,
                                   const BufferRef& value) {
  return true;
}

bool HttpListener::onMessageHeaderEnd() {
  //.
  return true;
}

bool HttpListener::onMessageContent(const BufferRef& chunk) {
  //.
  return true;
}

bool HttpListener::onMessageEnd() {
  //.
  return true;
}

void HttpListener::onProtocolError(HttpStatus code,
                                   const std::string& message) {
  //.
}

}  // namespace xzero
