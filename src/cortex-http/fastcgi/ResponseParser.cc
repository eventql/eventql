// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/fastcgi/ResponseParser.h>
#include <cortex-http/HttpListener.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/StringUtil.h>
#include <cortex-base/logging.h>

namespace cortex {
namespace http {
namespace fastcgi {

#define TRACE(msg...) do { \
  logTrace("fastcgi", "ResponseParser: " msg); \
} while (0)

// {{{ ResponseParser::StreamState impl
ResponseParser::StreamState::StreamState()
    : listener(nullptr),
      totalBytesReceived(0),
      contentFullyReceived(false),
      http1Parser(http1::Parser::MESSAGE, nullptr/*listener*/) {
}

ResponseParser::StreamState::~StreamState() {
}

void ResponseParser::StreamState::reset() {
  listener = nullptr;
  totalBytesReceived = 0;
  contentFullyReceived = false;
  http1Parser.reset();
}

void ResponseParser::StreamState::setListener(HttpListener* listener) {
  this->listener = listener;
  this->http1Parser.setListener(listener);
}
// }}}

ResponseParser::StreamState& ResponseParser::registerStreamState(int requestId) {
  if (streams_.find(requestId)  != streams_.end())
    RAISE(RuntimeError,
          "FastCGI stream with requestID %d already available.",
          requestId);

  return streams_[requestId];
}

void ResponseParser::removeStreamState(int requestId) {
  auto i = streams_.find(requestId);
  if (i != streams_.end()) {
    streams_.erase(i);
  }
}

ResponseParser::ResponseParser(
    std::function<HttpListener*(int requestId)> onCreateChannel,
    std::function<void(int requestId, int recordId)> onUnknownPacket,
    std::function<void(int requestId, const BufferRef& content)> onStdErr)
    : onCreateChannel_(onCreateChannel),
      onUnknownPacket_(onUnknownPacket),
      onStdErr_(onStdErr) {
}

void ResponseParser::reset() {
  streams_.clear();
}

size_t ResponseParser::parseFragment(const Record& record) {
  return parseFragment(BufferRef((const char*) &record, record.size()));
}

size_t ResponseParser::parseFragment(const BufferRef& chunk) {
  size_t readOffset = 0;

  // process each fully received packet ...
  while (readOffset + sizeof(fastcgi::Record) <= chunk.size()) {
    const fastcgi::Record* record =
        reinterpret_cast<const fastcgi::Record*>(chunk.data() + readOffset);

    if (chunk.size() - readOffset < record->size()) {
      break; // not enough bytes to process (next) full packet
    }

    readOffset += record->size();

    process(record);
  }
  return readOffset;
}

ResponseParser::StreamState& ResponseParser::getStream(int requestId) {
  auto s = streams_.find(requestId);
  if (s != streams_.end())
    return s->second;

  StreamState& stream = streams_[requestId];
  stream.setListener(onCreateChannel_(requestId));
  return stream;
}

void ResponseParser::process(const fastcgi::Record* record) {
  switch (record->type()) {
    case fastcgi::Type::StdOut:
      return streamStdOut(record);
    case fastcgi::Type::StdErr:
      return streamStdErr(record);
    case fastcgi::Type::GetValues:
      //return getValues(...);
    default:
      onUnknownPacket_(record->requestId(), (int) record->type());
      ;//write<fastcgi::UnknownTypeRecord>(record->type(), record->requestId());
  }
}

void ResponseParser::streamStdOut(const fastcgi::Record* record) {
  TRACE("streamStdOut: %zu", record->contentLength());

  StreamState& stream = getStream(record->requestId());
  stream.totalBytesReceived += record->size();

  if (record->contentLength() == 0)
    stream.contentFullyReceived = true;

  BufferRef content(record->content(), record->contentLength());
  stream.http1Parser.parseFragment(content);

  if (stream.contentFullyReceived) {
    TRACE("streamStdOut: onMessageEnd");
    stream.listener->onMessageEnd();
  }
}

void ResponseParser::streamStdErr(const fastcgi::Record* record) {
  TRACE("streamStdErr: %zu", record->contentLength());

  StreamState& stream = getStream(record->requestId());
  stream.totalBytesReceived += record->size();

  if (onStdErr_) {
    BufferRef content(record->content(), record->contentLength());
    onStdErr_(record->requestId(), content);
  }
}

} // namespace fastcgi
} // namespace http
} // namespace cortex
