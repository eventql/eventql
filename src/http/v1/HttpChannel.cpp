#include <xzero/http/v1/HttpChannel.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpTransport.h>
#include <xzero/Tokenizer.h>

namespace xzero {
namespace http1 {

HttpChannel::HttpChannel(HttpTransport* transport, const HttpHandler& handler,
                         std::unique_ptr<xzero::HttpInput>&& input)
    : xzero::HttpChannel(transport, handler,
                         std::move(input)),
      persistent_(false),
      connectionOptions_() {
}

HttpChannel::~HttpChannel() {
}

void HttpChannel::reset() {
  connectionOptions_.clear();
  xzero::HttpChannel::reset();
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

bool HttpChannel::onMessageHeader(const BufferRef& name,
                                  const BufferRef& value) {
  if (!iequals(name, "Connection"))
    return xzero::HttpChannel::onMessageHeader(name, value);

  std::vector<BufferRef> options = Tokenizer<BufferRef>::tokenize(value, ", ");

  for (const BufferRef& option: options) {
    connectionOptions_.push_back(option.str());

    if (iequals(option, "Keep-Alive"))
      persistent_ = true;
    else if (iequals(option, "close"))
      persistent_ = false;
  }

  return true;
}

bool HttpChannel::onMessageHeaderEnd() {
  // hide transport-level header fields
  request_->headers().remove("Connection");
  for (const auto& name: connectionOptions_)
    request_->headers().remove(name);

  return xzero::HttpChannel::onMessageHeaderEnd();
}

void HttpChannel::onProtocolError(HttpStatus code, const std::string& message) {
  if (!response_->isCommitted()) {
    persistent_ = false;

    if (request_->version() != HttpVersion::UNKNOWN)
      response_->setVersion(request_->version());
    else
      response_->setVersion(HttpVersion::VERSION_0_9);

    response_->sendError(code, message);
  } else {
    transport_->abort();
  }
}

} // namespace http1
} // namespace xzero
