#include <xzero/http/v1/HttpGenerator.h>
#include <xzero/http/HttpDateGenerator.h>
#include <xzero/http/HttpRequestInfo.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/net/EndPointWriter.h>
#include <xzero/io/FileRef.h>
#include <xzero/sysconfig.h>

namespace xzero {
namespace http1 {

HttpGenerator::HttpGenerator(HttpDateGenerator* dateGenerator)
    : dateGenerator_(dateGenerator),
      contentLength_(static_cast<size_t>(-1)),
      chunked_(false),
      buffer_() {
}

void HttpGenerator::recycle() {
  buffer_.clear();
}

void HttpGenerator::generateRequest(const HttpRequestInfo& info,
                                    const BufferRef& chunk, bool last,
                                    EndPointWriter* output) {
  generateRequestLine(info);
  generateHeaders(info);
  flushBuffer(output);
  generateBody(chunk, last, output);
}

void HttpGenerator::generateResponse(const HttpResponseInfo& info,
                                     const BufferRef& chunk, bool last,
                                     EndPointWriter* output) {
  generateResponseLine(info);

  if (static_cast<int>(info.status()) >= 200) {
    if (dateGenerator_) {
      buffer_.push_back("Date: ");
      dateGenerator_->fill(&buffer_);
      buffer_.push_back("\r\n");
    }

    generateHeaders(info);
  } else {
    buffer_.push_back("\r\n");
  }

  flushBuffer(output);
  generateBody(chunk, last, output);
}

void HttpGenerator::generateBody(const BufferRef& chunk, bool last,
                                 EndPointWriter* output) {
  if (chunked_) {
    int n;
    char buf[12];

    if (chunk.size() > 0) {
      n = snprintf(buf, sizeof(buf), "%zx\r\n", chunk.size());
      output->write(BufferRef(buf, static_cast<size_t>(n)));
      output->write(chunk);
      output->write(BufferRef("\r\n"));
    }

    if (last) {
      n = snprintf(buf, sizeof(buf), "0\r\n\r\n");
      output->write(BufferRef(buf, n));
    }
  } else {
    if (chunk.size() <= contentLength_) {
      contentLength_ -= chunk.size();
      output->write(chunk);
    } else {
      throw std::runtime_error("HTTP body chunk exceeds content length.");
    }
  }
}

void HttpGenerator::generateBody(FileRef&& chunk, bool last,
                                 EndPointWriter* output) {
  if (chunked_) {
    int n;
    char buf[12];

    if (chunk.size() > 0) {
      n = snprintf(buf, sizeof(buf), "%zx\r\n", chunk.size());
      output->write(BufferRef(buf, static_cast<size_t>(n)));
      output->write(std::move(chunk));
      output->write(BufferRef("\r\n"));
    }

    if (last) {
      n = snprintf(buf, sizeof(buf), "0\r\n\r\n");
      output->write(BufferRef(buf, n));
    }
  } else {
    if (chunk.size() <= contentLength_) {
      contentLength_ -= chunk.size();
      output->write(std::move(chunk));
    } else {
      throw std::runtime_error("HTTP body chunk exceeds content length.");
    }
  }
}

void HttpGenerator::generateRequestLine(const HttpRequestInfo& info) {
  buffer_.push_back(info.method());
  buffer_.push_back(' ');
  buffer_.push_back(info.entity());

  switch (info.version()) {
    case HttpVersion::VERSION_0_9:
      buffer_.push_back(" HTTP/0.9\r\n");
      break;
    case HttpVersion::VERSION_1_0:
      buffer_.push_back(" HTTP/1.0\r\n");
      break;
    case HttpVersion::VERSION_1_1:
      buffer_.push_back(" HTTP/1.1\r\n");
      break;
    default:
      throw std::runtime_error("Illegal State");
  }
}

void HttpGenerator::generateResponseLine(const HttpResponseInfo& info) {
  switch (info.version()) {
    case HttpVersion::VERSION_0_9:
      buffer_.push_back("HTTP/0.9 ");
      break;
    case HttpVersion::VERSION_1_0:
      buffer_.push_back("HTTP/1.0 ");
      break;
    case HttpVersion::VERSION_1_1:
      buffer_.push_back("HTTP/1.1 ");
      break;
    default:
      throw std::runtime_error("Illegal state.");
  }

  buffer_.push_back(static_cast<int>(info.status()));

  if (info.reason().size() > 0) {
    buffer_.push_back(' ');
    buffer_.push_back(info.reason());
  }

  buffer_.push_back("\r\n");
}

void HttpGenerator::generateHeaders(const HttpInfo& info) {
  chunked_ = info.hasContentLength() == false;
  contentLength_ = info.contentLength();

  for (const HeaderField& header: info.headers()) {
    buffer_.push_back(header.name());
    buffer_.push_back(": ");
    buffer_.push_back(header.value());
    buffer_.push_back("\r\n");
  }

  if (chunked_) {
    buffer_.push_back("Transfer-Encoding: chunked\r\n");
  } else {
    buffer_.push_back("Content-Length: ");
    buffer_.push_back(info.contentLength());
    buffer_.push_back("\r\n");
  }

  buffer_.push_back("\r\n");
}

void HttpGenerator::flushBuffer(EndPointWriter* output) {
  output->write(std::move(buffer_));
  buffer_.clear();
}

}  // namespace http1
}  // namespace
