#include <xzero/http/HttpChannel.h>
#include <xzero/http/HttpTransport.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpVersion.h>
#include <xzero/http/HttpInputListener.h>
#include <xzero/sysconfig.h>

namespace xzero {

HttpChannel::HttpChannel(HttpTransport* transport, const HttpHandler& handler,
                         std::unique_ptr<HttpInput>&& input)
    : state_(HttpChannelState::IDLE),
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
    response_->setCommitted(true);

    if (request_->expect100Continue()) {
      response_->send100Continue();
    }

    const bool isHeadReq = request_->method() == "HEAD";
    HttpResponseInfo info(request_->version(), response_->status(),
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
                                 const BufferRef& entity, int versionMajor,
                                 int versionMinor) {
  request_->setMethod(method.str());
  request_->setPath(entity.str());

  HttpVersion version = HttpVersion::UNKNOWN;
  switch (versionMajor * 10 + versionMinor) {
    case 20:
      version = HttpVersion::VERSION_2_0;
      break;
    case 11:
      version = HttpVersion::VERSION_1_1;
      break;
    case 10:
      version = HttpVersion::VERSION_1_0;
      break;
    case 9:
      version = HttpVersion::VERSION_0_9;
      break;
    default:
      break;
  }

  request_->setVersion(version);
  response_->setVersion(version);

  return true;
}

bool HttpChannel::onMessageHeader(const BufferRef& name,
                                  const BufferRef& value) {
  request_->headers().push_back(name.str(), value.str());

  if (iequals(name, "Expect") && iequals(value, "100-continue"))
    request_->setExpect100Continue(true);

  return true;
}

bool HttpChannel::onMessageHeaderEnd() {
  state_ = HttpChannelState::READY;
  return true;
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

void HttpChannel::onProtocolError(const BufferRef& chunk, size_t offset) {
  // TODO: make this an exception to be catched and respond as 400 Bad Request.
  /* printf("protocol error at offset %zu in chunk %p\n", offset, chunk.data()); */
  transport_->abort();
}

void HttpChannel::completed() {
  if (!response_->isCommitted()) {
    // commit headers with empty body
    response_->setContentLength(0);
    send(BufferRef(), nullptr);
  }

  transport_->completed();
}

// TODO: clear out wheather we need run() and HttpChannelState
void HttpChannel::run() {
  switch (state_) {
    case HttpChannelState::READY:
      state_ = HttpChannelState::HANDLING;
      handler_(request(), response());
      break;
    case HttpChannelState::HANDLING:
    default:
      break;
  }
}

}  // namespace xzero
