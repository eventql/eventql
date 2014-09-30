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

HttpOutput::HttpOutput(HttpChannel* channel)
    : channel_(channel),
      filters_() {
}

HttpOutput::~HttpOutput() {
}

void HttpOutput::recycle() {
  filters_.clear();
}

void HttpOutput::addFilter(HttpOutputFilter* filter) {
  if (channel_->response()->isCommitted())
    throw std::runtime_error("Invalid State. Cannot add output filters after commit.");

  filters_.push_back(filter);
}

void HttpOutput::removeAllFilters() {
  if (channel_->response()->isCommitted())
    throw std::runtime_error("Invalid State. Cannot clear output filters after commit.");

  filters_.clear();
}

void HttpOutput::completed() {
  channel_->completed();
}

void HttpOutput::write(const char* cstr, CompletionHandler&& completed) {
  const size_t slen = strlen(cstr);

  if (filters_.empty()) {
    write(BufferRef(cstr, slen), std::move(completed));
  } else {
    Buffer output;
    HttpOutputFilter::applyFilters(filters_, BufferRef(cstr, slen), &output);
    channel_->send(std::move(output), std::move(completed));
  }
}

void HttpOutput::write(const std::string& str, CompletionHandler&& completed) {
  if (filters_.empty()) {
    write(Buffer(str), std::move(completed));
  } else {
    BufferRef input(str);
    Buffer output;
    HttpOutputFilter::applyFilters(filters_, input, &output);
    channel_->send(std::move(output), std::move(completed));
  }
}

void HttpOutput::write(Buffer&& data, CompletionHandler&& completed) {
  if (filters_.empty()) {
    channel_->send(std::move(data), std::move(completed));
  } else {
    Buffer output;
    HttpOutputFilter::applyFilters(filters_, data.ref(), &output);
    channel_->send(std::move(output), std::move(completed));
  }
}

void HttpOutput::write(const BufferRef& data, CompletionHandler&& completed) {
  if (filters_.empty()) {
    channel_->send(data, std::move(completed));
  } else {
    Buffer output;
    HttpOutputFilter::applyFilters(filters_, data, &output);
    channel_->send(std::move(output), std::move(completed));
  }
}

void HttpOutput::write(FileRef&& file, CompletionHandler&& completed) {
  if (filters_.empty()) {
    channel_->send(std::move(file), std::move(completed));
  } else {
#if defined(HAVE_PREAD)
    Buffer input(file.size());
    ssize_t n = pread(file.handle(), input.data(), file.size(), file.offset());
    if (n < 0)
      throw std::system_error(errno, std::system_category());

    if (n != file.size())
      throw std::runtime_error("Did not read all required bytes from file.");

    input.resize(n);
#else
# error "Implementation missing"
#endif

    Buffer output;
    HttpOutputFilter::applyFilters(filters_, input, &output);

    channel_->send(std::move(output), std::move(completed));
  }
}

}  // namespace xzero
