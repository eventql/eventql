// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpRequest.h>
#include <xzero/logging/LogSource.h>

namespace xzero {

#ifndef NDEBUG
static LogSource requestLogger("http1.HttpRequest");
# define DEBUG(msg...) do { requestLogger.debug(msg); } while (0)
# define TRACE(msg...) do { requestLogger.trace(msg); } while (0)
#else
# define DEBUG(msg...) do {} while (0)
# define TRACE(msg...) do {} while (0)
#endif

HttpRequest::HttpRequest()
    : HttpRequest("", "", HttpVersion::UNKNOWN, false, {}, nullptr) {
  // .
}

HttpRequest::HttpRequest(std::unique_ptr<HttpInput>&& input)
    : HttpRequest("", "", HttpVersion::UNKNOWN, false, {}, std::move(input)) {
  // .
}

HttpRequest::HttpRequest(const std::string& method, const std::string& path,
                         HttpVersion version, bool secure,
                         const HeaderFieldList& headers,
                         std::unique_ptr<HttpInput>&& input)
    : method_(to_method(method)),
      unparsedMethod_(method),
      path_(path),
      version_(version),
      secure_(secure),
      expect100Continue_(false),
      headers_(headers),
      input_(std::move(input)) {
  // .
}

void HttpRequest::setMethod(const std::string& value) {
  unparsedMethod_ = value;
  method_ = to_method(value);
}

void HttpRequest::recycle() {
  TRACE("%p recycle", this);
  method_ = HttpMethod::UNKNOWN_METHOD;
  unparsedMethod_.clear();
  unparsedUri_.clear();
  path_.clear();
  version_ = HttpVersion::UNKNOWN;
  secure_ = false;
  headers_.reset();
  host_.clear();
  input_->recycle();
}

bool HttpRequest::setUri(const std::string& uri) {
  unparsedUri_ = uri;

  if (unparsedUri_ == "*") {
    path_ = "*";
    directoryDepth_ = 0;
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

#if !defined(NDEBUG)
  static const char* uriStateNames[] = {
      "Content",    "Slash",      "Dot",        "DotDot",
      "QuoteStart", "QuoteChar2", "QueryStart", };
#endif

  const char* i = unparsedUri_.data();
  const char* e = i + unparsedUri_.size() + 1;

  int depth = 0;
  int minDepth = 0;
  UriState state = UriState::Content;
  UriState quotedState;
  unsigned char decodedChar;
  char ch = *i++;

#if !defined(NDEBUG)  // suppress uninitialized warning
  quotedState = UriState::Content;
  decodedChar = '\0';
#endif

  while (i != e) {
    // TRACE("parse-uri: ch:%c, i:%c, state:%s, depth:%d", ch, *i,
    //       uriStateNames[(int)state], depth);

    switch (state) {
      case UriState::Content:
        switch (ch) {
          case '/':
            state = UriState::Slash;
            path_ += ch;
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
            path_ += ch;
            ch = *i++;
            break;
        }
        break;
      case UriState::Slash:
        switch (ch) {
          case '/':  // repeated slash "//"
            path_ += ch;
            ch = *i++;
            break;
          case '.':  // "/."
            state = UriState::Dot;
            path_ += ch;
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
            path_ += ch;
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
            path_ += ch;
            ch = *i++;
            break;
          case '.':
            // "/.."
            state = UriState::DotDot;
            path_ += ch;
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
            path_ += ch;
            ch = *i++;
            ++depth;
            break;
        }
        break;
      case UriState::DotDot:
        switch (ch) {
          case '/':
            path_ += ch;
            ch = *i++;
            --depth;
            minDepth = std::min(depth, minDepth);

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
            path_ += ch;
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

        DEBUG("Failed decoding Request-URI.");
        return false;
      case UriState::QuoteChar2:
        if (ch >= '0' && ch <= '9') {
          ch = decodedChar | (ch - '0');
          TRACE("parse-uri: decoded character 0x%02x", ch & 0xFF);

          switch (ch) {
            case '\0':
              DEBUG("Client attempted to inject ASCII-0 into Request-URI.");
              return false;
            case '%':
              state = UriState::Content;
              path_ += ch;
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

          TRACE("parse-uri: decoded character 0x%02x", ch & 0xFF);

          switch (ch) {
            case '\0':
              DEBUG("Client attempted to inject ASCII-0 into Request-URI.");
              return false;
            case '%':
              state = UriState::Content;
              path_ += ch;
              ch = *i++;
              break;
            default:
              state = quotedState;
              break;
          }

          break;
        }

        DEBUG("Failed decoding Request-URI.");
        return false;
      case UriState::QueryStart:
        if (ch == '?') {
          // skip repeative "?"'s
          ch = *i++;
          break;
        }

        // XXX (i - 1) because i points to the next byte already
        query_ = unparsedUri_.substr((i - unparsedUri_.data()) - 1, e - i);
        goto done;
      default:
        throw std::runtime_error("Internal error. Unhandled URI-parse state.");
    }
  }

  switch (state) {
    case UriState::QuoteStart:
    case UriState::QuoteChar2:
      DEBUG("Failed decoding Request-URI.");
      return false;
    default:
      break;
  }

done:
  TRACE("parse-uri(%s): success. path:%s, query:%s, depth:%d, mindepth:%d, state:%s",
        unparsedUri_.c_str(),
        path_.c_str(), query_.c_str(), depth, minDepth, uriStateNames[(int)state]);
  directoryDepth_ = depth;

  if (minDepth < 0) {
    return false;
  }

  return true;
}

void HttpRequest::setHost(const std::string& value) {
  host_ = value;
}

}  // namespace xzero
