// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/fastcgi/Generator.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-base/net/ByteArrayEndPoint.h>
#include <cortex-base/StringUtil.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/logging.h>
#include <string>

namespace cortex {
namespace http {
namespace fastcgi {

#ifndef NDEBUG
#define TRACE(msg...) logTrace("http.fastcgi.Generator", msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

// THOUGHTS:
//
// - maybe splitup generator into request/response generator, too

Generator::Generator(int requestId, EndPointWriter* writer)
    : mode_(Nothing),
      requestId_(requestId),
      bytesTransmitted_(0),
      buffer_(),
      writer_(writer) {
}

void Generator::generateRequest(const HttpRequestInfo& info) {
  mode_ = GenerateRequest;

  CgiParamStreamWriter paramsWriter;
  paramsWriter.encode("GATEWAY_INTERFACE", "CGI/1.1");
  paramsWriter.encode("SERVER_SOFTWARE", "cortex-http");
  paramsWriter.encode("SERVER_PROTOCOL", to_string(info.version()));
  if (info.headers().contains("Host"))
    paramsWriter.encode("SERVER_NAME", info.headers().get("Host"));
  paramsWriter.encode("REQUEST_METHOD", info.method());
  paramsWriter.encode("REQUEST_URI", info.entity());

  // TODO: we should set these too when talking to a PHP-FPM server for example
  // paramsWriter.encode("QUERY_STRING", "");
  // paramsWriter.encode("SCRIPT_NAME", "");
  // paramsWriter.encode("SCRIPT_FILENAME", "");
  // paramsWriter.encode("PATH_INFO", "");
  // paramsWriter.encode("PATH_TRANSLATED", "");
  // paramsWriter.encode("DOCUMENT_ROOT", "");
  // paramsWriter.encode("CONTENT_TYPE", info.headers().get("Content-Type"));
  // paramsWriter.encode("CONTENT_LENGTH", info.headers().get("Content-Length"));
  // paramsWriter.encode("HTTPS", "on");
  // paramsWriter.encode("REDIRECT_STATUS", "200"); // for PHP with --force-redirect (such as in: Gentoo/Linux)
  // paramsWriter.encode("REMOTE_USER", "");
  // paramsWriter.encode("REMOTE_ADDR", "");
  // paramsWriter.encode("REMOTE_PORT", "");
  // paramsWriter.encode("SERVER_ADDR", "");
  // paramsWriter.encode("SERVER_PORT", "");

  for (const HeaderField& header: info.headers()) {
    paramsWriter.encodeHeader(header.name(), header.value());
  }

  const Buffer& params = paramsWriter.output();
  write(Type::Params, requestId_, params.data(), params.size());
  write(Type::Params, requestId_, "", 0);
}

void Generator::generateRequest(const HttpRequestInfo& info, Buffer&& chunk) {
  generateRequest(info);
  generateBody(chunk);
}

void Generator::generateRequest(const HttpRequestInfo& info, const BufferRef& chunk) {
  generateRequest(info);
  generateBody(chunk);
}

void Generator::generateResponse(const HttpResponseInfo& info) {
  TRACE("generateResponse! status=%d %s",
        static_cast<int>(info.status()),
        to_string(info.status()).c_str());

  mode_ = GenerateResponse;

  Buffer payload;

  payload.push_back("Status: ");
  payload.push_back(std::to_string(static_cast<int>(info.status())));
  payload.push_back("\r\n");

  for (const HeaderField& header: info.headers()) {
    TRACE("  %s: %s", header.name().c_str(), header.value().c_str());
    payload.push_back(header.name());
    payload.push_back(": ");
    payload.push_back(header.value());
    payload.push_back("\r\n");
  }
  payload.push_back("\r\n");

  write(Type::StdOut, requestId_, payload.data(), payload.size());
}

void Generator::generateResponse(const HttpResponseInfo& info, const BufferRef& chunk) {
  generateResponse(info);
  generateBody(chunk);
}

void Generator::generateResponse(const HttpResponseInfo& info, Buffer&& chunk) {
  generateResponse(info);
  generateBody(std::move(chunk));
}

void Generator::generateResponse(const HttpResponseInfo& info, FileRef&& chunk) {
  generateResponse(info);
  generateBody(std::move(chunk));
}

void Generator::generateBody(Buffer&& chunk) {
  if (!chunk.empty()) {
    Type bodyType = mode_ == GenerateRequest ? Type::StdIn : Type::StdOut;
    write(bodyType, requestId_, chunk.data(), chunk.size());
  }
}

void Generator::generateBody(const BufferRef& chunk) {
  if (!chunk.empty()) {
    Type bodyType = mode_ == GenerateRequest ? Type::StdIn : Type::StdOut;
    write(bodyType, requestId_, chunk.data(), chunk.size());
  }
}

void Generator::generateBody(FileRef&& chunk) {
  if (chunk.empty())
    return;

  constexpr size_t chunkSizeCap = 0xFFFF;
  const Type bodyType = mode_ == GenerateRequest ? Type::StdIn : Type::StdOut;
  const size_t len = chunk.size();
  size_t offset = 0;

  for (;;) {
    size_t clen = std::min(len, chunkSizeCap + offset) - offset;

    // header
    Record header(bodyType, requestId_, clen, 0);
    buffer_.push_back(&header, sizeof(header));
    flushBuffer();

    // body
    if (offset + clen < len) {
      FileRef weak(chunk.handle(), chunk.offset() + offset, clen, false);
      bytesTransmitted_ += clen;
      offset += clen;
      writer_->write(std::move(weak));
    } else {
      // writes the last chunk, with close-flag preserved
      chunk.setOffset(chunk.offset() + offset);
      chunk.setSize(clen);
      bytesTransmitted_ += clen;
      writer_->write(std::move(chunk));
      break;
    }
  }
}

void Generator::generateEnd() {
  TRACE("generateEnd()");

  // exit mark for request/response stream
  switch (mode_) {
    case GenerateRequest: {
      Record eos(Type::StdIn, requestId_, 0, 0);
      buffer_.push_back(&eos, sizeof(eos));
      break;
    }
    case GenerateResponse: {
      Record eos(Type::StdOut, requestId_, 0, 0);
      buffer_.push_back(&eos, sizeof(eos));

      int appStatus = 0;
      ProtocolStatus protocolStatus = ProtocolStatus::RequestComplete;
      EndRequestRecord endRequest(requestId_, appStatus, protocolStatus);
      buffer_.push_back(&endRequest, sizeof(endRequest));
      break;
    }
    default: {
      break;
    }
  }

  flushBuffer();
}

void Generator::write(Type type, int requestId, const char* buf, size_t len) {
  TRACE("write<%s>(rid=%zu, len=%zu)", to_string(type).c_str(), requestId, len);

  if (len != 0) {
    constexpr size_t chunkSizeCap = 0xFFFF;
    constexpr char padding[8] = {0};

    for (size_t offset = 0; offset < len;) {
      size_t clen = std::min(len, chunkSizeCap + offset) - offset;
      size_t plen =
          clen % sizeof(padding) ? sizeof(padding) - clen % sizeof(padding) : 0;

      Record header(type, requestId, clen, plen);
      buffer_.push_back(&header, sizeof(header));
      buffer_.push_back(buf + offset, clen);
      buffer_.push_back(padding, plen);

      offset += clen;
    }
  } else {
    Record header(type, requestId, 0, 0);
    buffer_.push_back(&header, sizeof(header));
  }
}

void Generator::flushBuffer() {
  TRACE("flushBuffer: %zu bytes", buffer_.size());
  if (!buffer_.empty()) {
    bytesTransmitted_ += buffer_.size();
    writer_->write(std::move(buffer_));
    buffer_.clear();
  }
}

} // namespace fastcgi
} // namespace http
} // namespace cortex
