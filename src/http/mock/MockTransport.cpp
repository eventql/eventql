#include <xzero/http/mock/MockTransport.h>
#include <xzero/http/HttpHandler.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpInput.h>
#include <xzero/http/HttpChannel.h>
#include <xzero/http/BadMessage.h>
#include <xzero/executor/Executor.h>
#include <xzero/Buffer.h>
#include <xzero/io/FileRef.h>
#include <stdexcept>
#include <system_error>

namespace xzero {

// {{{ MockInput
class MockInput : public HttpInput {
 public:
  MockInput();
  int read(Buffer* result) override;
  size_t readLine(Buffer* result) override;
  void onContent(const BufferRef& chunk) override;
  void recycle() override;

 private:
  Buffer buffer_;
};

MockInput::MockInput()
    : buffer_() {
}

int MockInput::read(Buffer* result) {
  size_t n = buffer_.size();
  result->push_back(buffer_);
  buffer_.clear();
  return n;
}

size_t MockInput::readLine(Buffer* result) {
  return 0; // TODO
}

void MockInput::onContent(const BufferRef& chunk) {
  buffer_ += chunk;
}

void MockInput::recycle() {
  buffer_.clear();
}
// }}}

MockTransport::MockTransport(Executor* executor, const HttpHandler& handler)
    : MockTransport(executor, handler, 32, 64) {
}

MockTransport::MockTransport(Executor* executor,
                             const HttpHandler& handler,
                             size_t maxRequestUriLength,
                             size_t maxRequestBodyLength)
    : HttpTransport(nullptr /*endpoint*/),
      executor_(executor),
      handler_(handler),
      maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      isAborted_(false),
      isCompleted_(false),
      channel_(),
      responseInfo_(),
      responseBody_() {
}

MockTransport::~MockTransport() {
}

void MockTransport::run(HttpVersion version, const std::string& method,
                        const std::string& entity,
                        const HeaderFieldList& headers,
                        const std::string& body) {
  isCompleted_ = false;
  isAborted_ = false;

  std::unique_ptr<MockInput> input(new MockInput());
  channel_.reset(new HttpChannel(this, handler_, std::move(input),
                                 maxRequestUriLength_, maxRequestBodyLength_));


  try {
    channel_->onMessageBegin(method, entity, version);
    for (const auto& header: headers) {
      channel_->onMessageHeader(header.name(), header.value());
    }
    channel_->onMessageHeaderEnd();
    channel_->onMessageContent(BufferRef(body.data(), body.size()));
    channel_->onMessageEnd();
  } catch (const BadMessage& e) {
    channel_->response()->sendError(e.code(), e.what());
  }
}

void MockTransport::abort() {
  isAborted_ = true;
}

void MockTransport::completed() {
  isCompleted_ = true;
}

void MockTransport::send(HttpResponseInfo&& responseInfo,
                         const BufferRef& chunk,
                         CompletionHandler&& onComplete) {
  responseInfo_ = std::move(responseInfo);
  responseBody_ += chunk;

  if (onComplete) {
    executor_->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::send(const BufferRef& chunk, CompletionHandler&& onComplete) {
  responseBody_ += chunk;

  if (onComplete) {
    executor_->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::send(FileRef&& chunk, CompletionHandler&& onComplete) {
  responseBody_.reserve(chunk.size());

  ssize_t n = pread(chunk.handle(), responseBody_.end(), chunk.size(),
                    chunk.offset());

  if (n < 0)
    std::system_error(errno, std::system_category());

  if (n != chunk.size())
    std::runtime_error("Unexpected read count.");

  responseBody_.resize(responseBody_.size() + n);

  if (onComplete) {
    executor_->execute([onComplete]() {
      onComplete(true);
    });
  }
}

void MockTransport::onOpen() {
  HttpTransport::onOpen();
}

void MockTransport::onClose() {
  HttpTransport::onClose();
}

void MockTransport::onFillable() {
}

void MockTransport::onFlushable() {
}

bool MockTransport::onReadTimeout() {
  return HttpTransport::onReadTimeout();
}

} // namespace mock
