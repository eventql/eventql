#include <xzero/http/mock/MockTransport.h>
#include <xzero/http/HttpHandler.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpInput.h>
#include <xzero/http/HttpChannel.h>
#include <xzero/executor/Executor.h>
#include <xzero/Buffer.h>

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
    : HttpTransport(nullptr /*endpoint*/),
      executor_(executor),
      handler_(handler),
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
  std::unique_ptr<MockInput> input(new MockInput());
  channel_.reset(new HttpChannel(this, handler_, std::move(input)));

  int versionMajor = 0;
  int versionMinor = 0;
  switch (version) {
    case HttpVersion::VERSION_1_1:
      versionMajor = 1;
      versionMinor = 1;
      break;
    case HttpVersion::VERSION_1_0:
      versionMajor = 1;
      versionMinor = 0;
      break;
    case HttpVersion::VERSION_0_9:
      versionMajor = 0;
      versionMinor = 9;
      break;
    default:
      throw std::runtime_error("Invalid argument");
  }

  channel_->onMessageBegin(method, entity, versionMajor, versionMinor);
  for (const auto& header: headers) {
    channel_->onMessageHeader(header.name(), header.value());
  }
  channel_->onMessageHeaderEnd();
  channel_->onMessageContent(BufferRef(body.data(), body.size()));
  channel_->onMessageEnd();
}

void MockTransport::abort() {
}

void MockTransport::completed() {
}

void MockTransport::send(HttpResponseInfo&& responseInfo,
                         const BufferRef& chunk,
                         CompletionHandler&& onComplete) {
  responseInfo_ = std::move(responseInfo);
  responseBody_ += chunk;

  executor_->execute([onComplete]() {
    if (onComplete) {
      onComplete(true);
    }
  });
}

void MockTransport::send(const BufferRef& chunk, CompletionHandler&& onComplete) {
}

void MockTransport::send(FileRef&& chunk, CompletionHandler&& onComplete) {
}

void MockTransport::onOpen() {
}

void MockTransport::onClose() {
}

void MockTransport::onFillable() {
}

void MockTransport::onFlushable() {
}

bool MockTransport::onReadTimeout() {
  return HttpTransport::onReadTimeout();
}

} // namespace mock
