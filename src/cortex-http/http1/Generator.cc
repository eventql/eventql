// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/http1/Generator.h>
#include <cortex-http/HttpDateGenerator.h>
#include <cortex-http/HttpRequestInfo.h>
#include <cortex-http/HttpResponseInfo.h>
#include <cortex-http/HttpStatus.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-base/io/FileRef.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/sysconfig.h>

namespace cortex {
namespace http {
namespace http1 {

Generator::Generator(EndPointWriter* output)
    : bytesTransmitted_(0),
      contentLength_(Buffer::npos),
      chunked_(false),
      buffer_(),
      writer_(output) {
}

void Generator::recycle() {
  buffer_.clear();
  bytesTransmitted_ = 0;
}

void Generator::generateRequest(const HttpRequestInfo& info,
                                    Buffer&& chunk) {
  generateRequestLine(info);
  generateHeaders(info);
  flushBuffer();
  generateBody(std::move(chunk));
}

void Generator::generateRequest(const HttpRequestInfo& info,
                                    const BufferRef& chunk) {
  generateRequestLine(info);
  generateHeaders(info);
  flushBuffer();
  generateBody(chunk);
}

void Generator::generateResponse(const HttpResponseInfo& info,
                                     const BufferRef& chunk) {
  generateResponseInfo(info);
  generateBody(chunk);
  flushBuffer();
}

void Generator::generateResponse(const HttpResponseInfo& info,
                                     Buffer&& chunk) {
  generateResponseInfo(info);
  generateBody(std::move(chunk));
  flushBuffer();
}

void Generator::generateResponse(const HttpResponseInfo& info,
                                     FileRef&& chunk) {
  generateResponseInfo(info);
  generateBody(std::move(chunk));
}

void Generator::generateResponseInfo(const HttpResponseInfo& info) {
  generateResponseLine(info);

  if (static_cast<int>(info.status()) >= 200) {
    generateHeaders(info);
  } else {
    buffer_.push_back("\r\n");
  }

  flushBuffer();
}

void Generator::generateBody(const BufferRef& chunk) {
  if (chunked_) {
    if (chunk.size() > 0) {
      Buffer buf(12);
      buf.printf("%zx\r\n", chunk.size());
      writer_->write(std::move(buf));
      writer_->write(chunk);
      writer_->write(BufferRef("\r\n"));
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

void Generator::generateTrailer(const HeaderFieldList& trailers) {
  if (chunked_) {
    buffer_.push_back("0\r\n");
    for (const HeaderField& header: trailers) {
      buffer_.push_back(header.name());
      buffer_.push_back(": ");
      buffer_.push_back(header.value());
      buffer_.push_back("\r\n");
    }
    buffer_.push_back("\r\n");
  }
  flushBuffer();
}

void Generator::generateBody(Buffer&& chunk) {
  if (chunked_) {
    if (chunk.size() > 0) {
      Buffer buf(12);
      buf.printf("%zx\r\n", chunk.size());
      writer_->write(std::move(buf));
      writer_->write(std::move(chunk));
      writer_->write(BufferRef("\r\n"));
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

void Generator::generateBody(FileRef&& chunk) {
  if (chunked_) {
    int n;
    char buf[12];

    if (chunk.size() > 0) {
      n = snprintf(buf, sizeof(buf), "%zx\r\n", chunk.size());
      bytesTransmitted_ += n + chunk.size() + 2;
      writer_->write(BufferRef(buf, static_cast<size_t>(n)));
      writer_->write(std::move(chunk));
      writer_->write(BufferRef("\r\n"));
    }
  } else {
    if (chunk.size() <= contentLength_) {
      bytesTransmitted_ += chunk.size();
      contentLength_ -= chunk.size();
      writer_->write(std::move(chunk));
    } else {
      RAISE(RuntimeError, "HTTP body chunk exceeds content length.");
    }
  }
}

void Generator::generateRequestLine(const HttpRequestInfo& info) {
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

void Generator::generateResponseLine(const HttpResponseInfo& info) {
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
      RAISE(IllegalStateError, "Invalid HTTP version.");
  }

  buffer_.push_back(static_cast<int>(info.status()));

  buffer_.push_back(' ');
  if (info.reason().size() > 0)
    buffer_.push_back(info.reason());
  else
    buffer_.push_back(to_string(info.status()));

  buffer_.push_back("\r\n");
}

void Generator::generateHeaders(const HttpInfo& info) {
  chunked_ = info.hasContentLength() == false || info.hasTrailers();
  contentLength_ = info.contentLength();

  for (const HeaderField& header: info.headers()) {
    buffer_.push_back(header.name());
    buffer_.push_back(": ");
    buffer_.push_back(header.value());
    buffer_.push_back("\r\n");
  }

  if (!info.trailers().empty()) {
    buffer_.push_back("Trailer: ");
    size_t count = 0;
    for (const HeaderField& trailer: info.trailers()) {
      if (count)
        buffer_.push_back(", ");

      buffer_.push_back(trailer.name());
      count++;
    }
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

void Generator::flushBuffer() {
  if (!buffer_.empty()) {
    bytesTransmitted_ += buffer_.size();
    writer_->write(std::move(buffer_));
    buffer_.clear();
  }
}

}  // namespace http1
}  // namespace http
}  // namespace cortex
