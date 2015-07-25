// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/fastcgi/RequestParser.h>
#include <cortex-http/HttpListener.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/StringUtil.h>
#include <cortex-base/logging.h>

namespace cortex {
namespace http {
namespace fastcgi {

#ifndef NDEBUG
#define TRACE(msg...) logTrace("http.fastcgi.RequestParser",  msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

// {{{ RequestParser::StreamState impl
RequestParser::StreamState::StreamState()
    : listener(nullptr),
      totalBytesReceived(0),
      paramsFullyReceived(false),
      contentFullyReceived(false),
      params(),
      headers(),
      body() {
}

RequestParser::StreamState::~StreamState() {
}

void RequestParser::StreamState::reset() {
  listener = nullptr;
  totalBytesReceived = 0;
  paramsFullyReceived = false;
  contentFullyReceived = false;
  params.reset();
  headers.reset();
  body.clear();
}

void RequestParser::StreamState::onParam(
    const char *name, size_t nameLen,
    const char *value, size_t valueLen) {
  std::string nam(name, nameLen);
  std::string val(value, valueLen);

  if (StringUtil::beginsWith(nam, "HTTP_")) {
    // strip off "HTTP_" prefix and replace all '_' with '-'
    nam = nam.substr(5);
    StringUtil::replaceAll(&nam, "_", "-");

    headers.push_back(nam, val);
    return;
  }

  // non-HTTP-header parameters
  params.push_back(nam, val);

  // SERVER_PORT
  // SERVER_ADDR
  // SERVER_PROTOCOL
  // SERVER_SOFTWARE
  // SERVER_NAME (aka. Host request header)
  //
  // REMOTE_ADDR
  // REMOTE_PORT
  // REMOTE_IDENT
  //
  // HTTPS
  //
  // REQUEST_METHOD
  // REQUEST_URI
  //
  // AUTH_TYPE
  // QUERY_STRING
  //
  // DOCUMENT_ROOT
  // PATH_TRANSLATED
  // PATH_INFO
  // SCRIPT_FILENAME
  //
  // CONTENT_TYPE
  // CONTENT_LENGTH
}
// }}}

RequestParser::StreamState* RequestParser::registerStreamState(int requestId) {
  if (streams_.find(requestId)  != streams_.end())
    RAISE(RuntimeError,
          "FastCGI stream with requestID %d already available.",
          requestId);

  return &streams_[requestId];
}

RequestParser::StreamState* RequestParser::getStream(int requestId) {
  auto s = streams_.find(requestId);
  if (s != streams_.end())
    return &s->second;

  return nullptr;
}

void RequestParser::removeStreamState(int requestId) {
  auto i = streams_.find(requestId);
  if (i != streams_.end()) {
    streams_.erase(i);
  }
}

RequestParser::RequestParser(
    std::function<HttpListener*(int requestId, bool keepAlive)> onCreateChannel,
    std::function<void(int requestId, int recordId)> onUnknownPacket,
    std::function<void(int requestId)> onAbortRequest)
    : onCreateChannel_(onCreateChannel),
      onUnknownPacket_(onUnknownPacket),
      onAbortRequest_(onAbortRequest) {
}

void RequestParser::reset() {
  streams_.clear();
}

size_t RequestParser::parseFragment(const Record& record) {
  return parseFragment(BufferRef((const char*) &record, record.size()));
}

size_t RequestParser::parseFragment(const BufferRef& chunk) {
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

void RequestParser::process(const fastcgi::Record* record) {
  switch (record->type()) {
    case fastcgi::Type::BeginRequest:
      return beginRequest(static_cast<const fastcgi::BeginRequestRecord*>(record));
    case fastcgi::Type::AbortRequest:
      return abortRequest(static_cast<const fastcgi::AbortRequestRecord*>(record));
    case fastcgi::Type::Params:
      return streamParams(record);
    case fastcgi::Type::StdIn:
      return streamStdIn(record);
    case fastcgi::Type::Data:
      return streamData(record);
    case fastcgi::Type::GetValues:
      //return getValues(...);
    default:
      onUnknownPacket_(record->requestId(), (int) record->type());
      ;//write<fastcgi::UnknownTypeRecord>(record->type(), record->requestId());
  }
}

void RequestParser::beginRequest(const fastcgi::BeginRequestRecord* record) {
  TRACE("BeginRequest(role=%s, rid=%d, keepalive=%s)",
        record->role_str(),
        record->requestId(),
        record->isKeepAlive() ? "yes" : "no");

  if (auto stream = registerStreamState(record->requestId())) {
    stream->totalBytesReceived += record->size();

    stream->listener =
        onCreateChannel_(record->requestId(), record->isKeepAlive());
  }
}

void RequestParser::streamParams(const fastcgi::Record* record) {
  TRACE("Params(size=%zu)", record->contentLength());
  StreamState* stream = getStream(record->requestId());
  if (!stream)
    return;

  stream->totalBytesReceived += record->size();

  if (record->contentLength()) {
    stream->processParams(record->content(), record->contentLength());
    return;
  }

  TRACE("Params: fully received!");
  stream->paramsFullyReceived = true;

  for (const auto& param: stream->params)
    TRACE("  %s: %s", param.name().c_str(), param.value().c_str());

  BufferRef method(stream->params.get("REQUEST_METHOD"));
  BufferRef entity(stream->params.get("REQUEST_URI"));

  HttpVersion version = HttpVersion::UNKNOWN;
  const std::string& serverProtocol = stream->params.get("SERVER_PROTOCOL");
  if (serverProtocol == "HTTP/1.1")
    version = HttpVersion::VERSION_1_1;
  else if (serverProtocol == "HTTP/1.0")
    version = HttpVersion::VERSION_1_0;
  else if (serverProtocol == "HTTP/0.9")
    version = HttpVersion::VERSION_0_9;

  stream->listener->onMessageBegin(method, entity, version);

  for (const auto& header: stream->headers) {
    stream->listener->onMessageHeader(BufferRef(header.name()),
                                      BufferRef(header.value()));
  }

  stream->listener->onMessageHeaderEnd();

  if (!stream->body.empty()) {
    TRACE("Params: onMessageContent");
    stream->listener->onMessageContent(stream->body);
  }

  if (stream->paramsFullyReceived && stream->contentFullyReceived) {
    TRACE("Params: onMessageEnd");
    stream->listener->onMessageEnd();
  }
}

void RequestParser::streamStdIn(const fastcgi::Record* record) {
  StreamState* stream = getStream(record->requestId());
  if (!stream)
    return;

  stream->totalBytesReceived += record->size();

  TRACE("streamStdIn: payload:%zu, paramsEOS:%s",
      record->contentLength(),
      stream->paramsFullyReceived ? "true" : "false");

  BufferRef content(record->content(), record->contentLength());
  stream->listener->onMessageContent(content);

  if (record->contentLength() == 0)
    stream->contentFullyReceived = true;

  if (stream->paramsFullyReceived && stream->contentFullyReceived) {
    TRACE("streamStdIn: onMessageEnd");
    stream->listener->onMessageEnd();
  }
}

void RequestParser::streamData(const fastcgi::Record* record) {
  TRACE("streamData: %zu", record->contentLength());

  StreamState* stream = getStream(record->requestId());
  if (!stream)
    return;

  stream->totalBytesReceived += record->size();

  // ignore data-stream, as we've no association in HTTP-layer for it.
}

void RequestParser::abortRequest(const fastcgi::AbortRequestRecord* record) {
  TRACE("abortRequest");

  StreamState* stream = getStream(record->requestId());
  if (!stream)
    return;

  stream->totalBytesReceived += record->size();

  onAbortRequest_(record->requestId());

  removeStreamState(record->requestId());
}

} // namespace fastcgi
} // namespace http
} // namespace cortex
