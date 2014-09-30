#include <xzero/http/HttpOutput.h>
#include <xzero/http/HttpChannel.h>
#include <xzero/io/FileRef.h>
#include <cstring>

namespace xzero {

HttpOutput::HttpOutput(HttpChannel* channel)
    : channel_(channel) {
}

HttpOutput::~HttpOutput() {
}

void HttpOutput::recycle() {
}

void HttpOutput::completed() {
  channel_->completed();
}

void HttpOutput::write(const char* cstr, CompletionHandler&& completed) {
  const size_t slen = strlen(cstr);
  write(BufferRef(cstr, slen), std::forward<CompletionHandler>(completed));
}

void HttpOutput::write(const std::string& str, CompletionHandler&& completed) {
  write(Buffer(str), std::forward<CompletionHandler>(completed));
}

void HttpOutput::write(Buffer&& data, CompletionHandler&& completed) {
  channel_->send(std::move(data), std::forward<CompletionHandler>(completed));
}

void HttpOutput::write(const BufferRef& data, CompletionHandler&& completed) {
  channel_->send(data, std::forward<CompletionHandler>(completed));
}

void HttpOutput::write(FileRef&& file, CompletionHandler&& completed) {
  channel_->send(std::forward<FileRef>(file),
                 std::forward<CompletionHandler>(completed));
}

}  // namespace xzero
