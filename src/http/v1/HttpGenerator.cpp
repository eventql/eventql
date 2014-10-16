#include <xzero/http/v1/HttpGenerator.h>
#include <xzero/http/HttpDateGenerator.h>
#include <xzero/http/HttpRequestInfo.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpStatus.h>
#include <xzero/net/EndPointWriter.h>
#include <xzero/io/FileRef.h>
#include <xzero/sysconfig.h>

namespace xzero {
namespace http1 {

HttpGenerator::HttpGenerator(HttpDateGenerator* dateGenerator,
                             EndPointWriter* output)
    : dateGenerator_(dateGenerator),
      writer_(output),
      contentLength_(static_cast<size_t>(-1)),
      chunked_(false),
      buffer_() {
}

void HttpGenerator::recycle() {
  buffer_.clear();
}

void HttpGenerator::generateRequest(const HttpRequestInfo& info,
                                    Buffer&& chunk, bool last) {
  generateRequestLine(info);
  generateHeaders(info);
  flushBuffer();
  generateBody(std::move(chunk), last);
}

void HttpGenerator::generateRequest(const HttpRequestInfo& info,
                                    const BufferRef& chunk, bool last) {
  generateRequestLine(info);
  generateHeaders(info);
  flushBuffer();
  generateBody(chunk, last);
}

void HttpGenerator::generateResponse(const HttpResponseInfo& info,
                                     const BufferRef& chunk, bool last) {
  generateResponseInfo(info);
  generateBody(chunk, last);
  flushBuffer();
}

void HttpGenerator::generateResponse(const HttpResponseInfo& info,
                                     Buffer&& chunk, bool last) {
  generateResponseInfo(info);
  generateBody(std::move(chunk), last);
  flushBuffer();
}

void HttpGenerator::generateResponse(const HttpResponseInfo& info,
                                     FileRef&& chunk, bool last) {
  generateResponseInfo(info);
  generateBody(std::move(chunk), last);
}

void HttpGenerator::generateResponseInfo(const HttpResponseInfo& info) {
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

  flushBuffer();
}

void HttpGenerator::generateBody(const BufferRef& chunk, bool last) {
  if (chunked_) {
    if (chunk.size() > 0) {
      Buffer buf(12);
      buf.printf("%zx\r\n", chunk.size());
      writer_->write(std::move(buf));
      writer_->write(chunk);
      writer_->write(BufferRef("\r\n"));
    }

    if (last) {
      writer_->write(BufferRef("0\r\n\r\n"));
    }
  } else {
    if (chunk.size() <= contentLength_) {
      contentLength_ -= chunk.size();
      writer_->write(chunk);
    } else {
      throw std::runtime_error("HTTP body chunk exceeds content length.");
    }
  }
}

void HttpGenerator::generateEnd() {
  if (chunked_) {
    writer_->write(BufferRef("0\r\n\r\n"));
  }
}

void HttpGenerator::generateBody(Buffer&& chunk, bool last) {
  if (chunked_) {
    if (chunk.size() > 0) {
      Buffer buf(12);
      buf.printf("%zx\r\n", chunk.size());
      writer_->write(std::move(buf));
      writer_->write(std::move(chunk));
      writer_->write(BufferRef("\r\n"));
    }

    if (last) {
      writer_->write(BufferRef("0\r\n\r\n"));
    }
  } else {
    if (chunk.size() <= contentLength_) {
      contentLength_ -= chunk.size();
      writer_->write(std::move(chunk));
    } else {
      throw std::runtime_error("HTTP body chunk exceeds content length.");
    }
  }
}

void HttpGenerator::generateBody(FileRef&& chunk, bool last) {
  if (chunked_) {
    int n;
    char buf[12];

    if (chunk.size() > 0) {
      n = snprintf(buf, sizeof(buf), "%zx\r\n", chunk.size());
      writer_->write(BufferRef(buf, static_cast<size_t>(n)));
      writer_->write(std::move(chunk));
      writer_->write(BufferRef("\r\n"));
    }

    if (last) {
      n = snprintf(buf, sizeof(buf), "0\r\n\r\n");
      writer_->write(BufferRef(buf, n));
    }
  } else {
    if (chunk.size() <= contentLength_) {
      contentLength_ -= chunk.size();
      writer_->write(std::move(chunk));
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

  buffer_.push_back(' ');
  if (info.reason().size() > 0)
    buffer_.push_back(info.reason());
  else
    buffer_.push_back(to_string(info.status()));

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

void HttpGenerator::flushBuffer() {
  writer_->write(std::move(buffer_));
  buffer_.clear();
}

}  // namespace http1
}  // namespace
