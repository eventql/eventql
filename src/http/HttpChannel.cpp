#include <xzero/http/HttpChannel.h>
#include <xzero/http/HttpTransport.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpVersion.h>
#include <xzero/http/HttpInputListener.h>
#include <xzero/http/BadMessage.h>
#include <xzero/sysconfig.h>

namespace xzero {

HttpChannel::HttpChannel(HttpTransport* transport, const HttpHandler& handler,
                         std::unique_ptr<HttpInput>&& input,
                         size_t maxRequestUriLength,
                         size_t maxRequestBodyLength)
    : maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      state_(HttpChannelState::IDLE),
      transport_(transport),
      request_(new HttpRequest(std::move(input))),
      response_(new HttpResponse(this, createOutput())),
      handler_(handler) {
  //.
}

HttpChannel::~HttpChannel() {
  //.
}

void HttpChannel::reset() {
  state_ = HttpChannelState::IDLE;
  request_->recycle();
  response_->recycle();
}

std::unique_ptr<HttpOutput> HttpChannel::createOutput() {
  return std::unique_ptr<HttpOutput>(new HttpOutput(this));
}

void HttpChannel::send(const BufferRef& data, CompletionHandler&& onComplete) {
  if (!response_->isCommitted()) {
    if (!response_->status())
      throw std::runtime_error("No HTTP response status set yet.");

    response_->setCommitted(true);

    if (request_->expect100Continue()) {
      response_->send100Continue();
    }

    const bool isHeadReq = request_->method() == "HEAD";
    HttpResponseInfo info(response_->version(), response_->status(),
                          response_->reason(), isHeadReq,
                          response_->contentLength(), response_->headers());

    if (!info.headers().contains("Server"))
      info.headers().push_back("Server", "xzero/" LIBXZERO_VERSION);

    transport_->send(std::move(info), data,
                     std::forward<CompletionHandler>(onComplete));
  } else {
    transport_->send(data, std::forward<CompletionHandler>(onComplete));
  }
}

void HttpChannel::send(FileRef&& file, CompletionHandler&& completed) {
  if (!response_->isCommitted()) {
    send(BufferRef(), nullptr); // commit headers with empty body
  }

  transport_->send(std::forward<FileRef>(file),
                   std::forward<CompletionHandler>(completed));
}

void HttpChannel::send100Continue() {
  if (!request()->expect100Continue())
    throw std::runtime_error("Illegal State. no 100-continue expected.");

  request()->setExpect100Continue(false);

  HttpResponseInfo info(request_->version(), HttpStatus::ContinueRequest,
                        "Continue", false, 0, {});

  transport_->send(std::move(info), BufferRef(), nullptr);
}

bool HttpChannel::onMessageBegin(const BufferRef& method,
                                 const BufferRef& entity,
                                 HttpVersion version) {
  response_->setVersion(version);
  request_->setVersion(version);
  request_->setMethod(method.str());
  if (!request_->setUri(entity.str())) {
    state_ = HttpChannelState::HANDLING;
    response_->sendError(HttpStatus::BadRequest);
    return false;
  }

  return true;
}

bool HttpChannel::onMessageHeader(const BufferRef& name,
                                  const BufferRef& value) {
  request_->headers().push_back(name.str(), value.str());

  if (iequals(name, "Expect") && iequals(value, "100-continue"))
    request_->setExpect100Continue(true);

  // rfc7230, Section 5.4, p2
  if (iequals(name, "Host")) {
    if (request_->host().empty())
      request_->setHost(value.str());
    else
      throw BadMessage(HttpStatus::BadRequest, "Multiple host headers are illegal.");
  }

  return true;
}

bool HttpChannel::onMessageHeaderEnd() {
  if (state_ != HttpChannelState::HANDLING) {
    state_ = HttpChannelState::HANDLING;

    // rfc7230, Section 5.4, p2
    if (request_->version() == HttpVersion::VERSION_1_1) {
      if (!request_->headers().contains("Host")) {
        throw BadMessage(HttpStatus::BadRequest, "No Host header given.");
      }
    }

    handleRequest();
  }
  return true;
}

void HttpChannel::handleRequest() {
  try {
    handler_(request(), response());
  } catch (std::exception& e) {
    response()->sendError(HttpStatus::InternalServerError, e.what());
  } catch (...) {
    response()->sendError(HttpStatus::InternalServerError,
                          "unhandled unknown exception");
  }
}

bool HttpChannel::onMessageContent(const BufferRef& chunk) {
  request_->input()->onContent(chunk);
  return true;
}

bool HttpChannel::onMessageEnd() {
  if (!request_->input())
    throw std::runtime_error("Internal Error. No HttpInput available?");

  if (request_->input()->listener())
    request_->input()->listener()->onAllDataRead();

  return false;
}

void HttpChannel::onProtocolError(HttpStatus code, const std::string& message) {
  response_->sendError(code, message);
}

void HttpChannel::completed() {
  if (!response_->isCommitted()) {
    // commit headers with empty body
    response_->setContentLength(0);
    send(BufferRef(), nullptr);
  }

  transport_->completed();
}

}  // namespace xzero
