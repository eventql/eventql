// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/HttpRequest.h>
#include <xzero/HttpRangeDef.h>
#include <xzero/HttpConnection.h>
#include <base/io/BufferSource.h>
#include <base/io/FilterSource.h>
#include <base/io/FileSource.h>
#include <base/io/ChunkedEncoder.h>
#include <base/DebugLogger.h>
#include <base/Process.h>  // Process::dumpCore()
#include <base/strutils.h>
#include <base/sysconfig.h>
#include <algorithm>
#include <array>
#include <strings.h>  // strcasecmp()
#include <stdlib.h>   // realpath()
#include <limits.h>   // PATH_MAX
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#if !defined(XZERO_NDEBUG)
#define TRACE(level, msg...) XZERO_DEBUG("HttpRequest", (level), msg)
#else
#define TRACE(msg...) \
  do {                \
  } while (0)
#endif

namespace xzero {

// {{{ helper methods
/**
 * converts a range-spec into real offsets.
 *
 * \todo Mark this fn as \c constexpr as soon Ubuntu's LTS default compiler
 *supports it (14.04)
 */
inline /*constexpr*/ std::pair<std::size_t, std::size_t> makeOffsets(
    const std::pair<std::size_t, std::size_t>& p, std::size_t actualSize) {
  return p.first == HttpRangeDef::npos
             ? std::make_pair(actualSize - p.second,
                              actualSize - 1)  // last N bytes
             : p.second == HttpRangeDef::npos && p.second > actualSize
                   ? std::make_pair(p.first, actualSize - 1)  // from fixed N to
                                                              // the end of file
                   : std::make_pair(p.first, p.second);       // fixed range
}

/**
 * generates a boundary tag.
 *
 * \return a value usable as boundary tag.
 */
inline std::string generateBoundaryID() {
  static const char* map = "0123456789abcdef";
  char buf[16 + 1];

  for (std::size_t i = 0; i < sizeof(buf) - 1; ++i)
    buf[i] = map[random() % (sizeof(buf) - 1)];

  buf[sizeof(buf) - 1] = '\0';

  return std::string(buf);
}
// }}}

char HttpRequest::statusCodes_[512][4];

HttpRequest::HttpRequest(HttpConnection& conn)
    : onPostProcess(),
      onRequestDone(),
      connection(conn),
      method(),
      unparsedUri(),
      path(),
      query(),
      pathinfo(),
      fileinfo(),
      httpVersionMajor(0),
      httpVersionMinor(0),
      hostname(),
      requestHeaders(),
      bytesTransmitted_(0),
      username(),
      documentRoot(),
      expectingContinue(false),
      // customData(),
      status(HttpStatus::Undefined),
      responseHeaders(),
      outputFilters(),
      inspectHandlers_(),
      hostid_(),
      directoryDepth_(0),
      errorHandler_(nullptr),
      timeStart_() {
#ifndef XZERO_NDEBUG
  static std::atomic<unsigned long long> rid(0);
  setLoggingPrefix("HttpRequest(%lld,%s:%d)", ++rid,
                   connection.remoteIP().c_str(), connection.remotePort());
#endif
}

HttpRequest::~HttpRequest() { TRACE(2, "destructing"); }

/**
 * Assigns unparsed URI to request and decodes it for path and query attributes.
 *
 * We also compute the directory depth for directory traversal detection.
 *
 * \see HttpRequest::path
 * \see HttpRequest::query
 * \see HttpRequest::testDirectoryTraversal()
 */
bool HttpRequest::setUri(const BufferRef& uri) {
  unparsedUri = uri;

  if (unparsedUri == "*") {
    // XXX this is a special case, described in RFC 2616, section 5.1.2
    path = "*";
    return true;
  }

  enum class UriState {
    Content,
    Slash,
    Dot,
    DotDot,
    QuoteStart,
    QuoteChar2,
    QueryStart,
  };

#if !defined(XZERO_NDEBUG)
  static const char* uriStateNames[] = {
      "Content",    "Slash",      "Dot",        "DotDot",
      "QuoteStart", "QuoteChar2", "QueryStart", };
#endif

  const char* i = unparsedUri.cbegin();
  const char* e = unparsedUri.cend() + 1;

  int depth = 0;
  UriState state = UriState::Content;
  UriState quotedState;
  unsigned char decodedChar;
  char ch = *i++;

#if !defined(XZERO_NDEBUG)  // suppress uninitialized warning
  quotedState = UriState::Content;
  decodedChar = '\0';
#endif

  while (i != e) {
    TRACE(2, "parse-uri: ch:%c, i:%c, state:%s, depth:%d", ch, *i,
          uriStateNames[(int)state], depth);

    switch (state) {
      case UriState::Content:
        switch (ch) {
          case '/':
            state = UriState::Slash;
            path << ch;
            ch = *i++;
            break;
          case '%':
            quotedState = state;
            state = UriState::QuoteStart;
            ch = *i++;
            break;
          case '?':
            state = UriState::QueryStart;
            ch = *i++;
            break;
          default:
            path << ch;
            ch = *i++;
            break;
        }
        break;
      case UriState::Slash:
        switch (ch) {
          case '/':  // repeated slash "//"
            path << ch;
            ch = *i++;
            break;
          case '.':  // "/."
            state = UriState::Dot;
            path << ch;
            ch = *i++;
            break;
          case '%':  // "/%"
            quotedState = state;
            state = UriState::QuoteStart;
            ch = *i++;
            break;
          case '?':  // "/?"
            state = UriState::QueryStart;
            ch = *i++;
            ++depth;
            break;
          default:
            state = UriState::Content;
            path << ch;
            ch = *i++;
            ++depth;
            break;
        }
        break;
      case UriState::Dot:
        switch (ch) {
          case '/':
            // "/./"
            state = UriState::Slash;
            path << ch;
            ch = *i++;
            break;
          case '.':
            // "/.."
            state = UriState::DotDot;
            path << ch;
            ch = *i++;
            break;
          case '%':
            quotedState = state;
            state = UriState::QuoteStart;
            ch = *i++;
            break;
          case '?':
            // "/.?"
            state = UriState::QueryStart;
            ch = *i++;
            ++depth;
            break;
          default:
            state = UriState::Content;
            path << ch;
            ch = *i++;
            ++depth;
            break;
        }
        break;
      case UriState::DotDot:
        switch (ch) {
          case '/':
            path << ch;
            ch = *i++;
            --depth;

            // the directory depth is optionally checked later, if needed.

            state = UriState::Slash;
            break;
          case '%':
            quotedState = state;
            state = UriState::QuoteStart;
            ch = *i++;
            break;
          default:
            state = UriState::Content;
            path << ch;
            ch = *i++;
            ++depth;
            break;
        }
        break;
      case UriState::QuoteStart:
        if (ch >= '0' && ch <= '9') {
          state = UriState::QuoteChar2;
          decodedChar = (ch - '0') << 4;
          ch = *i++;
          break;
        }

        // unsafe convert `ch` to lower-case character
        ch |= 0x20;

        if (ch >= 'a' && ch <= 'f') {
          state = UriState::QuoteChar2;
          decodedChar = (ch - ('a' + 10)) << 4;
          ch = *i++;
          break;
        }

        log(Severity::debug, "Failed decoding Request-URI.");
        return false;
      case UriState::QuoteChar2:
        if (ch >= '0' && ch <= '9') {
          ch = decodedChar | (ch - '0');
          log(Severity::debug, "parse-uri: decoded character 0x%02x",
              ch & 0xFF);

          switch (ch) {
            case '\0':
              log(Severity::notice,
                  "Client attempted to inject ASCII-0 into Request-URI.");
              return false;
            case '%':
              state = UriState::Content;
              path << ch;
              ch = *i++;
              break;
            default:
              state = quotedState;
              break;
          }
          break;
        }

        // unsafe convert `ch` to lower-case character
        ch |= 0x20;

        if (ch >= 'a' && ch <= 'f') {
          // XXX mathematically, a - b = -(b - a), because:
          //   a - b = a + (-b) = 1*a + (-1*b) = (-1*-a) + (-1*b) = -1 * (-a +
          // b) = -1 * (b - a) = -(b - a)
          // so a subtract 10 from a even though you wanted to intentionally to
          // add them, because it's 'a'..'f'
          // This should be better for the compiler than: `decodedChar + 'a' -
          // 10` as in the latter case it
          // is not garranteed that the compiler pre-calculates the constants.
          //
          // XXX we OR' the values together as we know that their bitfields do
          // not intersect.
          //
          ch = decodedChar | (ch - ('a' - 10));

          log(Severity::debug, "parse-uri: decoded character 0x%02x",
              ch & 0xFF);

          switch (ch) {
            case '\0':
              log(Severity::notice,
                  "Client attempted to inject ASCII-0 into Request-URI.");
              return false;
            case '%':
              state = UriState::Content;
              path << ch;
              ch = *i++;
              break;
            default:
              state = quotedState;
              break;
          }

          break;
        }

        log(Severity::notice, "Failed decoding Request-URI.");
        return false;
      case UriState::QueryStart:
        if (ch == '?') {
          // skip repeative "?"'s
          ch = *i++;
          break;
        }

        // XXX (i - 1) because i points to the next byte already
        query = unparsedUri.ref((i - unparsedUri.cbegin()) - 1, e - i);
        goto done;
      default:
        log(Severity::debug, "Internal error. Unhandled state");
        return false;
    }
  }

  switch (state) {
    case UriState::QuoteStart:
    case UriState::QuoteChar2:
      log(Severity::notice, "Failed decoding Request-URI.");
      return false;
    default:
      break;
  }

done:
  TRACE(2, "parse-uri: success. path:%s, query:%s, depth:%d, state:%s",
        path.c_str(), query.str().c_str(), depth, uriStateNames[(int)state]);
  directoryDepth_ = depth;
  return true;
}

/**
 * Computes the real fileinfo and pathinfo part.
 *
 * Modifies:
 * <ul>
 *   <li>pathinfo</li>
 *   <li>fileinfo</li>
 * </ul>
 */
void HttpRequest::updatePathInfo() {
  if (!fileinfo) return;

  // split "/the/tail" from "/path/to/script.php/the/tail"

  std::string fullname(fileinfo->path());
  size_t origpos = fullname.size() - 1;
  size_t pos = origpos;

  for (;;) {
    if (fileinfo->exists()) {
      if (pos != origpos)
        pathinfo = path.ref(path.size() - (origpos - pos + 1));

      break;
    }

    if (fileinfo->error() == ENOTDIR) {
      pos = fileinfo->path().rfind('/', pos - 1);
      fileinfo = connection.worker().fileinfo(fileinfo->path().substr(0, pos));
    } else {
      break;
    }
  }
}

BufferRef HttpRequest::requestHeader(const BufferRef& name) const {
  for (auto& i : requestHeaders)
    if (iequals(i.name, name)) return i.value;

  return BufferRef();
}

std::string HttpRequest::requestHeaderCumulative(
    const std::string& name) const {
  std::vector<BufferRef> fields;
  for (auto& i : requestHeaders)
    if (iequals(i.name, name)) fields.push_back(i.value);

  switch (fields.size()) {
    case 0:
      return std::string();
    case 1:
      return fields[0].str();
    default: {
      std::string result;
      result += fields[0];
      for (size_t i = 1, e = fields.size(); i != e; ++i) {
        result += "; ";
        result += fields[i];
      }
      return result;
    }
  }
}

void HttpRequest::removeRequestHeaders(
    const std::initializer_list<BufferRef>& names) {
  for (const auto& name : names) {
    auto i = std::find_if(
        requestHeaders.begin(), requestHeaders.end(),
        [&](const HttpRequestHeader& h) { return h.name == name; });
    if (i != requestHeaders.end()) {
      requestHeaders.erase(i);
    }
  }
}

std::string HttpRequest::cookie(const std::string& name) const {
  char* input = strdup(requestHeaderCumulative("Cookie").c_str());
  std::string result;

  for (char* str1 = input, *s1;; str1 = NULL) {
    char* kvpair = strtok_r(str1, ";", &s1);
    if (kvpair == NULL) break;

    char* value = nullptr;
    char* key = strtok_r(kvpair, "= ", &value);
    if (!key || strcmp(key, name.c_str()) != 0) continue;

    // strip leading spaces
    while (*value && (*value == ' ' || *value == '=')) ++value;

    // strip trailing spaces
    size_t vlen = strlen(value);
    while (isspace(value[vlen - 1])) --vlen;

    result = std::string(value, vlen);
    break;
  }
  free(input);
  return std::move(result);
}

std::string HttpRequest::hostid() const {
  if (hostid_.empty())
    hostid_ = make_hostid(hostname, connection.listener().port());

  return hostid_;
}

void HttpRequest::setHostid(const std::string& value) { hostid_ = value; }

/** serializes the HTTP response status line plus headers into a byte-stream.
 *
 * This method is invoked right before the response content is written or the
 * response is flushed at all.
 *
 * It first sets the status code (if not done yet), invoked post_process
 *callback,
 * performs connection-level response header modifications and then
 * builds the response chunk for status line and headers.
 *
 * Post-modification done <b>after</b> the post_process hook has been invoked:
 * <ol>
 *   <li>set status code to 200 (Ok) if not set yet.</li>
 *   <li>set Content-Type header to a default if not set yet.</li>
 *   <li>set Connection header to keep-alive or close (computed value)</li>
 *   <li>append Transfer-Encoding chunked if no Content-Length header is
 *set.</li>
 *   <li>optionally enable TCP_CORK if this is no keep-alive connection and the
 *administrator allows it.</li>
 * </ol>
 *
 * \note this does not serialize the message body.
 */
std::unique_ptr<Source> HttpRequest::serialize() {
  Buffer buffers;

  if (expectingContinue)
    status = HttpStatus::ExpectationFailed;
  else if (status == static_cast<HttpStatus>(0))
    status = HttpStatus::Ok;

  // post-response hook
  onPostProcess();
  connection.worker().server().onPostProcess(this);

  // setup (connection-level) response transfer
  if (supportsProtocol(1, 1) && !responseHeaders.contains("Content-Length") &&
      !responseHeaders.contains("Transfer-Encoding") &&
      !isResponseContentForbidden()) {
    responseHeaders.push_back("Transfer-Encoding", "chunked");
    outputFilters.push_back(std::make_shared<ChunkedEncoder>());
  }

  bool keepalive = connection.shouldKeepAlive();
  if (!connection.worker().server().maxKeepAlive()) keepalive = false;

  // remaining request count, that is allowed on a persistent connection
  std::size_t rlim = connection.worker().server().maxKeepAliveRequests();
  if (rlim) {
    rlim = connection.requestCount_ <= rlim
               ? rlim - connection.requestCount_ + 1
               : 0;

    if (rlim == 0)
      // disable keep-alive, if max request count has been reached
      keepalive = false;
  }

  if (supportsProtocol(1, 1))
    buffers.push_back("HTTP/1.1 ");
  else if (supportsProtocol(1, 0))
    buffers.push_back("HTTP/1.0 ");
  else
    buffers.push_back("HTTP/0.9 ");

  buffers.push_back(statusCodes_[static_cast<int>(status)]);
  buffers.push_back(' ');
  buffers.push_back(statusStr(status));
  buffers.push_back("\r\n");

  bool dateFound = false;
  bool hasServerHeader = false;

  for (auto& i : responseHeaders) {
    if (unlikely(iequals(i.name, "Date"))) {
      dateFound = true;
    } else if (unlikely(iequals(i.name, "Server"))) {
      hasServerHeader = true;
    }

    buffers.push_back(i.name.data(), i.name.size());
    buffers.push_back(": ");
    buffers.push_back(i.value.data(), i.value.size());
    buffers.push_back("\r\n");
  };

  if (likely(!dateFound)) {
    buffers.push_back("Date: ");
    buffers.push_back(connection.worker().now().http_str());
    buffers.push_back("\r\n");
  }

  if (connection.worker().server().advertise() &&
      !connection.worker().server().tag().empty()) {
    if (!hasServerHeader) {
      buffers.push_back("Server: ");
    }
    buffers.push_back(connection.worker().server().tag());
    buffers.push_back("\r\n");
  }

  // only set Connection-response-header if found as request-header, too
  if (!requestHeader("Connection").empty() ||
      keepalive != connection.shouldKeepAlive()) {
    if (keepalive) {
      buffers.push_back("Connection: keep-alive\r\n");

      if (rlim) {
        // sent keep-alive timeout and remaining request count
        buffers.printf("Keep-Alive: timeout=%lu, max=%zu\r\n",
                       static_cast<time_t>(
                           connection.worker().server().maxKeepAlive().value()),
                       rlim);
      } else {
        // sent keep-alive timeout only (infinite request count)
        buffers.printf(
            "Keep-Alive: timeout=%lu\r\n",
            static_cast<time_t>(
                connection.worker().server().maxKeepAlive().value()));
      }
    } else {
      buffers.push_back("Connection: close\r\n");
    }
  }

  buffers.push_back("\r\n");

  connection.setShouldKeepAlive(keepalive);

  if (connection.worker().server().tcpCork())
    connection.socket()->setTcpCork(true);

  return std::unique_ptr<BufferSource>(new BufferSource(std::move(buffers)));
}

void HttpRequest::write(const char* text) {
  if (text && *text) {
    Buffer buf;
    buf.push_back(text, strlen(text));
    write<BufferSource>(std::move(buf));
  }
}

/** write given source to response content and invoke the completion handler
 *when done.
 *
 * \param chunk the content (chunk) to push to the client
 * \param handler completion handler to invoke when source has been fully
 *flushed or if an error occured
 *
 * \note this implicitely flushes the response-headers if not yet done, thus,
 *making it impossible to modify them after this write.
 */
void HttpRequest::write(std::unique_ptr<Source>&& chunk) {
  if (unlikely(!connection.isOpen())) {
    return;
  }

  switch (connection.state()) {
    case HttpConnection::Undefined:
    case HttpConnection::ReadingRequest:
      // XXX bad state
      break;
    case HttpConnection::ProcessingRequest:
      connection.setState(HttpConnection::SendingReply);
      connection.write(serialize());
    // fall through
    case HttpConnection::SendingReply:
      if (outputFilters.empty())
        connection.write(std::move(chunk));
      else
        connection.write<FilterSource>(chunk.release(), &outputFilters, false);
      break;
    case HttpConnection::SendingReplyDone:
    case HttpConnection::KeepAliveRead:
      // XXX bad state
      break;
  }
}

/** populates a default-response content, possibly modifying a few response
 *headers, too.
 *
 * \return the newly created response content or a null ptr if the statuc code
 *forbids content.
 *
 * \note Modified headers are "Content-Type" and "Content-Length".
 */
void HttpRequest::writeDefaultResponseContent() {
  if (isResponseContentForbidden()) return;

  std::string cs = statusStr(status);
  char buf[1024];

  int nwritten = snprintf(buf, sizeof(buf),
                          "<html>"
                          "<head><title>%s</title></head>"
                          "<body><h1>%d %s</h1></body>"
                          "</html>\r\n",
                          cs.c_str(), status, cs.c_str());

  responseHeaders.overwrite("Content-Type", "text/html");

  char slen[64];
  snprintf(slen, sizeof(slen), "%d", nwritten);
  responseHeaders.overwrite("Content-Length", slen);

  write<BufferSource>(Buffer::fromCopy(buf, nwritten));
}

/*! appends a callback source into the output buffer if non-empty or invokes it
 *directly otherwise.
 *
 * Invoke this method to get called back (notified) when all preceding content
 *chunks have been
 * fully sent to the client already.
 *
 * This method either appends this callback into the output queue, thus, being
 *invoked when all
 * preceding output chunks have been handled so far, or the callback gets
 *invoked directly
 * when there is nothing in the output queue (meaning, that everything has been
 *already fully
 * sent to the client).
 *
 * \retval true The callback will be invoked later (callback appended to output
 *queue).
 * \retval false The output queue is empty (everything sent out so far *OR* the
 *connection is aborted) and the callback was invoked directly.
 */
bool HttpRequest::writeCallback(CallbackSource::Callback cb) {
  if (!connection.isOpen()) {
    cb();
    return false;
  }

  if (connection.isOutputPending()) {
    // acquire a ref in order to protect accidental destructions.
    connection.ref();

    connection.write<CallbackSource>([this, cb]() {
      // XXX we are already invoked from within the connection's worker thread,
      // so no need to re-post us into the same thread.
      cb();
      connection.unref();
    });
    return true;
  } else {
    cb();
    return false;
  }
}

std::array<std::string, 600> initialize_codes() {
  std::array<std::string, 600> codes;

  for (std::size_t i = 0; i < codes.size(); ++i) codes[i] = "Undefined";

  auto set = [&](HttpStatus st, const char* txt)
                 ->void { codes[static_cast<int>(st)] = txt; };

  set(HttpStatus::ContinueRequest, "Continue");
  set(HttpStatus::SwitchingProtocols, "Switching Protocols");
  set(HttpStatus::Processing, "Processing");

  set(HttpStatus::Ok, "Ok");
  set(HttpStatus::Created, "Created");
  set(HttpStatus::Accepted, "Accepted");
  set(HttpStatus::NonAuthoriativeInformation, "Non Authoriative Information");
  set(HttpStatus::NoContent, "No Content");
  set(HttpStatus::ResetContent, "Reset Content");
  set(HttpStatus::PartialContent, "Partial Content");

  set(HttpStatus::MultipleChoices, "Multiple Choices");
  set(HttpStatus::MovedPermanently, "Moved Permanently");
  set(HttpStatus::MovedTemporarily, "Moved Temporarily");
  set(HttpStatus::NotModified, "Not Modified");
  set(HttpStatus::TemporaryRedirect, "Temporary Redirect");
  set(HttpStatus::PermanentRedirect, "Permanent Redirect");

  set(HttpStatus::BadRequest, "Bad Request");
  set(HttpStatus::Unauthorized, "Unauthorized");
  set(HttpStatus::PaymentRequired, "Payment Required");
  set(HttpStatus::Forbidden, "Forbidden");
  set(HttpStatus::NotFound, "Not Found");
  set(HttpStatus::MethodNotAllowed, "Method Not Allowed");
  set(HttpStatus::NotAcceptable, "Not Acceptable");
  set(HttpStatus::ProxyAuthenticationRequired, "Proxy Authentication Required");
  set(HttpStatus::RequestTimeout, "Request Timeout");
  set(HttpStatus::Conflict, "Conflict");
  set(HttpStatus::Gone, "Gone");
  set(HttpStatus::LengthRequired, "Length Required");
  set(HttpStatus::PreconditionFailed, "Precondition Failed");
  set(HttpStatus::PayloadTooLarge, "Payload Too Large");
  set(HttpStatus::RequestUriTooLong, "Request URI Too Long");
  set(HttpStatus::UnsupportedMediaType, "Unsupported Media Type");
  set(HttpStatus::RequestedRangeNotSatisfiable,
      "Requested Range Not Satisfiable");
  set(HttpStatus::ExpectationFailed, "Expectation Failed");
  set(HttpStatus::ThereAreTooManyConnectionsFromYourIP,
      "There Are Too Many Connections From Your IP");
  set(HttpStatus::UnprocessableEntity, "Unprocessable Entity");
  set(HttpStatus::Locked, "Locked");
  set(HttpStatus::FailedDependency, "Failed Dependency");
  set(HttpStatus::UnorderedCollection, "Unordered Collection");
  set(HttpStatus::UpgradeRequired, "Upgrade Required");
  set(HttpStatus::PreconditionRequired, "Precondition Required");
  set(HttpStatus::TooManyRequests, "Too Many Requests");
  set(HttpStatus::RequestHeaderFieldsTooLarge,
      "Request Header Fields Too Large");
  set(HttpStatus::NoResponse, "No Response");

  set(HttpStatus::InternalServerError, "Internal Server Error");
  set(HttpStatus::NotImplemented, "Not Implemented");
  set(HttpStatus::BadGateway, "Bad Gateway");
  set(HttpStatus::ServiceUnavailable, "Service Unavailable");
  set(HttpStatus::GatewayTimeout, "Gateway Timedout");
  set(HttpStatus::HttpVersionNotSupported, "HTTP Version Not Supported");
  set(HttpStatus::VariantAlsoNegotiates, "Variant Also Negotiates");
  set(HttpStatus::InsufficientStorage, "Insufficient Storage");
  set(HttpStatus::LoopDetected, "Loop Detected");
  set(HttpStatus::BandwidthExceeded, "Bandwidth Exceeded");
  set(HttpStatus::NotExtended, "Not Extended");
  set(HttpStatus::NetworkAuthenticationRequired,
      "Network Authentication Required");

  return std::move(codes);
}

static std::array<std::string, 600> codes_ = initialize_codes();

std::string HttpRequest::statusStr(HttpStatus value) {
  return codes_[static_cast<size_t>(value)];
}

/** Finishes handling the current request.
 *
 * This results either in a closed underlying connection or the connection
 *waiting for the next request to arrive,
 * or, if pipelined, processing the next request right away.
 *
 * If finish() is invoked with no response being generated yet, it will be
 *generate a default response automatically,
 * depending on the \b status code set.
 *
 * \p finish() can be invoked even if there is still output pending, but it may
 *not be added more output to the connection
 * by the application after \e finish() has been invoked.
 *
 * \note We might also trigger the custom error page handler, if no content was
 *given.
 * \note This also queues the underlying connection for processing the next
 *request (on keep-alive).
 * \note This also clears out the client abort callback, as set with \p
 *setAbortHandler().
 *
 * \see HttpConnection::write(), HttpConnection::isOutputPending()
 * \see finalize()
 */
void HttpRequest::finish() {
  TRACE(2, "finish(): isOutputPending:%s, cstate:%s",
        connection.isOutputPending() ? "true" : "false",
        connection.state_str());

  setAbortHandler(nullptr);

  if (!connection.isOpen()) {
    connection.setState(HttpConnection::SendingReplyDone);
    finalize();
    return;
  }

  switch (connection.state()) {
    case HttpConnection::Undefined:
    // fall through (should never happen)
    case HttpConnection::ReadingRequest:
    // fall through (should never happen, should it?)
    case HttpConnection::ProcessingRequest:
      if (static_cast<int>(status) == 0) status = HttpStatus::NotFound;

      if (errorHandler_) {
        TRACE(2, "running custom error handler");
        // reset the handler right away to avoid endless nesting
        auto handler = errorHandler_;
        errorHandler_ = decltype(errorHandler_)();

        if (handler(this)) return;

        // the handler did not produce any response, so default to the
        // buildin-output
      }
      TRACE(2, "streaming default error content");

      if (isResponseContentForbidden()) {
        connection.write(serialize());
      } else if (status == HttpStatus::Ok) {
        if (method != "HEAD") {
          responseHeaders.overwrite("Content-Length", "0");
        }
        connection.write(serialize());
      } else {
        writeDefaultResponseContent();
      }
    /* fall through */
    case HttpConnection::SendingReply:
      // FIXME: can it become an issue when the response body may not be
      // non-empty
      // but we have outputFilters defined, thus, writing a single (possibly
      // empty?)
      // response body chunk?

      if (!outputFilters.empty()) {
        // mark the end of stream (EOS) by passing an empty chunk to the
        // outputFilters.
        connection.write<FilterSource>(&outputFilters, true);

        // FIXME: this EOS chunk doesn't get written always, i.e. when this
        // request is `Connection: closed` and the content is known ahead.
        // This causes problems with ObjectCache::Builder to get to know when
        // the stream has ended.
        // b/c onRequestEnd isn't invoked, ...
      }

      connection.setState(HttpConnection::SendingReplyDone);

      if (!connection.isOutputPending()) {
        // the response body is already fully transmitted, so finalize this
        // request object directly.
        finalize();
      } else {
        ;  // printf("HttpRequest.finish(): delay finalize() as we've output
           // pending\n");
      }
      break;
    case HttpConnection::SendingReplyDone:
#if !defined(XZERO_NDEBUG)
      log(Severity::error,
          "BUG: invalid invocation of finish() on a already finished request.");
      Process::dumpCore();
#endif
      break;
    case HttpConnection::KeepAliveRead:
      break;
  }
}

/** internally invoked when the response has been <b>fully</b> flushed to the
 *client and we're to pass control back to the underlying connection.
 *
 * The request's reply has been fully transmitted to the client, so we are to
 *invoke the request-done callback
 * and clear request-local custom data.
 *
 * After that, the connection gets either \p close()'d or will enter \p resume()
 *state, to receive and process the next request.
 *
 * \see finish()
 */
void HttpRequest::finalize() {
  TRACE(2, "finalize()");

  onRequestDone();
  connection.worker().server().onRequestDone(this);
  connection.clearRequestBody();
  clearCustomData();
  inspectHandlers_.clear();

  if (connection.isOpen() && connection.shouldKeepAlive()) {
    TRACE(2, "finalize: resuming");
    clear();
    connection.resume();
  } else {
    TRACE(2, "finalize: closing");
    connection.close();
  }
}

/** to be called <b>once</b> in order to initialize this class for
 *instanciation.
 *
 * \note to be invoked by HttpServer constructor.
 * \see server
 */
void HttpRequest::initialize() {
  // pre-compute string representations of status codes for use in response
  // serialization
  for (std::size_t i = 0; i < sizeof(statusCodes_) / sizeof(*statusCodes_); ++i)
    snprintf(statusCodes_[i], sizeof(*statusCodes_), "%03zu", i);
}

/** sets a callback to invoke on early connection aborts (by the remote end).
 *
 * This callback is only invoked when the client closed the connection before
 * \p HttpRequest::finish() has been invoked and completed already.
 *
 * Do <b>NOT</b> try to access the request object within the callback as it
 * might be destroyed already.
 *
 * \see HttpRequest::finish()
 */
void HttpRequest::setAbortHandler(std::function<void()>&& cb) {
  connection.clientAbortHandler_ = std::move(cb);

  if (connection.clientAbortHandler_) {
    // get notified on EOF at least (do not care about timeout handling)
    connection.wantRead(TimeSpan::Zero);
  }
}

/** Tests resolved path for directory traversal attempts outside document root.
 *
 * \retval true directory traversal outside document root detected and a Bad
 *Request response has been sent back to client.
 * \retval false no directory traversal outside document root detected or real
 *path could not be calculated.
 *
 * \note We assume \c fileinfo have been set already. Do \b NOT call this
 *function unless \c fileinfo has been set.
 * \note Call this function before you want to actually do something with a
 *requested file or directory.
 */
bool HttpRequest::testDirectoryTraversal() {
  if (directoryDepth_ >= 0) return false;

  log(Severity::warn, "directory traversal detected: %s",
      fileinfo->path().c_str());

  status = HttpStatus::BadRequest;
  finish();

  return true;
}

bool HttpRequest::sendfile() { return sendfile(fileinfo); }

bool HttpRequest::sendfile(const std::string& filename) {
  return sendfile(connection.worker().fileinfo(filename));
}

bool HttpRequest::sendfile(const HttpFileRef& transferFile) {
  status = verifyClientCache(transferFile);
  if (status != HttpStatus::Ok) return true;

  int fd = -1;
  if (equals(method, "GET")) {
    fd = transferFile->handle();

    if (fd < 0) {
      log(Severity::error, "Could not open file '%s': %s",
          transferFile->path().c_str(), strerror(errno));
      status = HttpStatus::Forbidden;
      return true;
    }
  } else if (!equals(method, "HEAD")) {
    status = HttpStatus::MethodNotAllowed;
    return true;
  }

  responseHeaders.push_back("Last-Modified", transferFile->lastModified());
  responseHeaders.push_back("ETag", transferFile->etag());

  if (!processRangeRequest(transferFile, fd)) {
    responseHeaders.push_back("Accept-Ranges", "bytes");
    responseHeaders.push_back("Content-Type", transferFile->mimetype());
    responseHeaders.push_back("Content-Length",
                              std::to_string(transferFile->size()));

    if (fd >= 0) {  // GET request
#if defined(HAVE_POSIX_FADVISE)
      posix_fadvise(fd, 0, transferFile->size(), POSIX_FADV_SEQUENTIAL);
#endif
      write<FileSource>(fd, 0, transferFile->size(), false);
    }
  }

  return true;
}

/**
 * verifies wether the client may use its cache or not.
 *
 * \param in request object
 */
HttpStatus HttpRequest::verifyClientCache(
    const HttpFileRef& transferFile) const {
  // If-None-Match
  do {
    BufferRef value = requestHeader("If-None-Match");
    if (value.empty()) continue;

    // XXX: on static files we probably don't need the token-list support
    if (value != transferFile->etag()) continue;

    return HttpStatus::NotModified;
  } while (0);

  // If-Modified-Since
  do {
    BufferRef value = requestHeader("If-Modified-Since");
    if (value.empty()) continue;

    DateTime dt(value);
    if (!dt.valid()) continue;

    if (transferFile->mtime() > dt.unixtime()) continue;

    return HttpStatus::NotModified;
  } while (0);

  // If-Match
  do {
    BufferRef value = requestHeader("If-Match");
    if (value.empty()) continue;

    if (value == "*") continue;

    // XXX: on static files we probably don't need the token-list support
    if (value == transferFile->etag()) continue;

    return HttpStatus::PreconditionFailed;
  } while (0);

  // If-Unmodified-Since
  do {
    BufferRef value = requestHeader("If-Unmodified-Since");
    if (value.empty()) continue;

    DateTime dt(value);
    if (!dt.valid()) continue;

    if (transferFile->mtime() <= dt.unixtime()) continue;

    return HttpStatus::PreconditionFailed;
  } while (0);

  return HttpStatus::Ok;
}

/*! fully processes the ranged requests, if one, or does nothing.
 *
 * \retval true this was a ranged request and we fully processed it (invoked
 *finish())
 * \internal false this is no ranged request. nothing is done on it.
 */
bool HttpRequest::processRangeRequest(const HttpFileRef& transferFile,
                                      int fd)  //{{{
{
  BufferRef range_value(requestHeader("Range"));
  HttpRangeDef range;

  // if no range request or range request was invalid (by syntax) we fall back
  // to a full response
  if (range_value.empty() || !range.parse(range_value)) return false;

  BufferRef ifRangeCond(requestHeader("If-Range"));
  if (!ifRangeCond.empty()) {
    // printf("If-Range specified: %s\n", ifRangeCond.str().c_str());
    if (!equals(ifRangeCond, transferFile->etag()) &&
        !equals(ifRangeCond, transferFile->lastModified())) {
      // printf("-> does not equal\n");
      return false;
    }
  }

  status = HttpStatus::PartialContent;

  if (range.size() > 1) {
    // generate a multipart/byteranged response, as we've more than one range to
    // serve

    std::unique_ptr<CompositeSource> content(new CompositeSource());
    Buffer buf;
    std::string boundary(generateBoundaryID());
    std::size_t contentLength = 0;

    for (int i = 0, e = range.size(); i != e; ++i) {
      std::pair<std::size_t, std::size_t> offsets(
          makeOffsets(range[i], transferFile->size()));
      if (offsets.second < offsets.first) {
        status = HttpStatus::RequestedRangeNotSatisfiable;
        return true;
      }

      std::size_t partLength = 1 + offsets.second - offsets.first;

      buf.clear();
      buf.push_back("\r\n--");
      buf.push_back(boundary);
      buf.push_back("\r\nContent-Type: ");
      buf.push_back(transferFile->mimetype());

      buf.push_back("\r\nContent-Range: bytes ");
      buf.push_back(offsets.first);
      buf.push_back("-");
      buf.push_back(offsets.second);
      buf.push_back("/");
      buf.push_back(transferFile->size());
      buf.push_back("\r\n\r\n");

      contentLength += buf.size() + partLength;

      if (fd >= 0) {
        bool lastChunk = i + 1 == e;
        content->push_back<BufferSource>(std::move(buf));
        content->push_back<FileSource>(fd, offsets.first, partLength,
                                       lastChunk);
      }
    }

    buf.clear();
    buf.push_back("\r\n--");
    buf.push_back(boundary);
    buf.push_back("--\r\n");

    contentLength += buf.size();
    content->push_back<BufferSource>(std::move(buf));

    // push the prepared ranged response into the client
    char slen[32];
    snprintf(slen, sizeof(slen), "%zu", contentLength);

    responseHeaders.push_back("Content-Type",
                              "multipart/byteranges; boundary=" + boundary);
    responseHeaders.push_back("Content-Length", slen);

    if (fd >= 0) {
      write(std::move(content));
    }
  } else {  // generate a simple (single) partial response
    std::pair<std::size_t, std::size_t> offsets(
        makeOffsets(range[0], transferFile->size()));
    if (offsets.second < offsets.first) {
      status = HttpStatus::RequestedRangeNotSatisfiable;
      return true;
    }

    responseHeaders.push_back("Content-Type", transferFile->mimetype());

    std::size_t length = 1 + offsets.second - offsets.first;

    char slen[32];
    snprintf(slen, sizeof(slen), "%zu", length);
    responseHeaders.push_back("Content-Length", slen);

    char cr[128];
    snprintf(cr, sizeof(cr), "bytes %zu-%zu/%zu", offsets.first, offsets.second,
             transferFile->size());
    responseHeaders.push_back("Content-Range", cr);

    if (fd >= 0) {
#if defined(HAVE_POSIX_FADVISE)
      posix_fadvise(fd, offsets.first, length, POSIX_FADV_SEQUENTIAL);
#endif
      write<FileSource>(fd, offsets.first, length, true);
    }
  }

  return true;
}  //}}}
}  // namespace xzero
