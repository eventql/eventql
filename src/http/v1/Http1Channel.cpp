#include <xzero/http/v1/Http1Channel.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpTransport.h>
#include <xzero/Tokenizer.h>

namespace xzero {
namespace http1 {

Http1Channel::Http1Channel(HttpTransport* transport,
                         const HttpHandler& handler,
                         std::unique_ptr<HttpInput>&& input,
                         size_t maxRequestUriLength,
                         size_t maxRequestBodyLength)
    : xzero::HttpChannel(transport, handler, std::move(input),
                         maxRequestUriLength, maxRequestBodyLength),
      persistent_(false),
      connectionOptions_() {
}

Http1Channel::~Http1Channel() {
}

void Http1Channel::reset() {
  connectionOptions_.clear();
  xzero::HttpChannel::reset();
}

bool Http1Channel::onMessageBegin(const BufferRef& method,
                                 const BufferRef& entity,
                                 HttpVersion version) {

  switch (version) {
    case HttpVersion::VERSION_1_1:
      persistent_ = true;
      break;
    case HttpVersion::VERSION_1_0:
    case HttpVersion::VERSION_0_9:
      persistent_ = false;
      break;
    default:
      throw std::runtime_error("Invalid State. Illegal version passed.");
  }

  return xzero::HttpChannel::onMessageBegin(method, entity, version);
}

bool Http1Channel::onMessageHeader(const BufferRef& name,
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

bool Http1Channel::onMessageHeaderEnd() {
  // hide transport-level header fields
  request_->headers().remove("Connection");
  for (const auto& name: connectionOptions_)
    request_->headers().remove(name);

  return xzero::HttpChannel::onMessageHeaderEnd();
}

void Http1Channel::onProtocolError(HttpStatus code, const std::string& message) {
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
