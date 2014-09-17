#include <xzero/http/v1/HttpChannel.h>

namespace xzero {
namespace http1 {

HttpChannel::HttpChannel(HttpTransport* transport, const HttpHandler& handler,
                         std::unique_ptr<xzero::HttpInput>&& input)
    : xzero::HttpChannel(transport, handler,
                         std::move(input)),
      persistent_(false) {
}

HttpChannel::~HttpChannel() {
}

bool HttpChannel::onMessageHeader(const BufferRef& name,
                                  const BufferRef& value) {
  if (iequals(name, "Connection")) {
    if (iequals(value, "keep-alive")) {
      persistent_ = true;
    } else if (iequals(value, "close")) {
      persistent_ = false;
    }
  }

  return xzero::HttpChannel::onMessageHeader(name, value);
}

bool HttpChannel::onMessageBegin(const BufferRef& method,
                                 const BufferRef& entity, int versionMajor,
                                 int versionMinor) {

  switch (versionMajor * 10 + versionMinor) {
    case 11:
      persistent_ = true;
      break;
    case 10:
      // no persist. unless explicitely specified via "Connection: keep-alive"
      persistent_ = false;
      break;
    case 9:
      persistent_ = false;
      break;
    default:
      break;
  }

  return xzero::HttpChannel::onMessageBegin(method, entity, versionMajor,
                                            versionMinor);
}

} // namespace http1
} // namespace xzero
