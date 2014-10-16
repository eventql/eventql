#include <xzero/http/HttpOutput.h>
#include <xzero/http/HttpOutputFilter.h>
#include <xzero/http/HttpChannel.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/io/FileRef.h>
#include <xzero/sysconfig.h>
#include <cstring>
#include <system_error>
#include <stdexcept>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace xzero {

HttpOutput::HttpOutput(HttpChannel* channel) : channel_(channel) {
}

HttpOutput::~HttpOutput() {
}

void HttpOutput::recycle() {
}

void HttpOutput::addFilter(std::shared_ptr<HttpOutputFilter> filter) {
  channel_->addOutputFilter(filter);
}

void HttpOutput::removeAllFilters() {
  channel_->removeAllOutputFilters();
}

void HttpOutput::completed() {
  channel_->completed();
}

void HttpOutput::write(const char* cstr, CompletionHandler&& completed) {
  const size_t slen = strlen(cstr);
  write(BufferRef(cstr, slen), std::move(completed));
}

void HttpOutput::write(const std::string& str, CompletionHandler&& completed) {
  write(Buffer(str), std::move(completed));
}

void HttpOutput::write(Buffer&& data, CompletionHandler&& completed) {
  channel_->send(std::move(data), std::move(completed));
}

void HttpOutput::write(const BufferRef& data, CompletionHandler&& completed) {
  channel_->send(data, std::move(completed));
}

void HttpOutput::write(FileRef&& input, CompletionHandler&& completed) {
  channel_->send(std::move(input), std::move(completed));
}

}  // namespace xzero
