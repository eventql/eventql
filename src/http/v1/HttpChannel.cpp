#include <xzero/http/v1/HttpChannel.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>

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

void HttpChannel::onProtocolError(const BufferRef& chunk, size_t offset) {
  printf("**** onProtocolError at %zu (%d)\n", offset, response_->isCommitted());
  if (!response_->isCommitted()) {
    persistent_ = false;

    if (request_->version() != HttpVersion::UNKNOWN)
      response_->setVersion(request_->version());
    else
      response_->setVersion(HttpVersion::VERSION_0_9);

    response_->setStatus(HttpStatus::BadRequest);
    response_->completed();
  } else {
    abort();
  }
}

} // namespace http1
} // namespace xzero
