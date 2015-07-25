// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/http1/Parser.h>
#include <cortex-http/HttpListener.h>
#include <cortex-http/HttpStatus.h>
#include <cortex-http/BadMessage.h>
#include <cortex-base/logging.h>

namespace cortex {
namespace http {
namespace http1 {

#if !defined(NDEBUG)
#define TRACE(fmt...) logTrace("http.http1.Parser", fmt)
#else
#define TRACE(msg...) do {} while (0)
#endif

std::string to_string(Parser::State state) {
  switch (state) {
    // artificial
    case Parser::PROTOCOL_ERROR:
      return "protocol-error";
    case Parser::MESSAGE_BEGIN:
      return "message-begin";

    // request-line
    case Parser::REQUEST_LINE_BEGIN:
      return "request-line-begin";
    case Parser::REQUEST_METHOD:
      return "request-method";
    case Parser::REQUEST_ENTITY_BEGIN:
      return "request-entity-begin";
    case Parser::REQUEST_ENTITY:
      return "request-entity";
    case Parser::REQUEST_PROTOCOL_BEGIN:
      return "request-protocol-begin";
    case Parser::REQUEST_PROTOCOL_T1:
      return "request-protocol-t1";
    case Parser::REQUEST_PROTOCOL_T2:
      return "request-protocol-t2";
    case Parser::REQUEST_PROTOCOL_P:
      return "request-protocol-p";
    case Parser::REQUEST_PROTOCOL_SLASH:
      return "request-protocol-slash";
    case Parser::REQUEST_PROTOCOL_VERSION_MAJOR:
      return "request-protocol-version-major";
    case Parser::REQUEST_PROTOCOL_VERSION_MINOR:
      return "request-protocol-version-minor";
    case Parser::REQUEST_LINE_LF:
      return "request-line-lf";
    case Parser::REQUEST_0_9_LF:
      return "request-0-9-lf";

    // Status-Line
    case Parser::STATUS_LINE_BEGIN:
      return "status-line-begin";
    case Parser::STATUS_PROTOCOL_BEGIN:
      return "status-protocol-begin";
    case Parser::STATUS_PROTOCOL_T1:
      return "status-protocol-t1";
    case Parser::STATUS_PROTOCOL_T2:
      return "status-protocol-t2";
    case Parser::STATUS_PROTOCOL_P:
      return "status-protocol-t2";
    case Parser::STATUS_PROTOCOL_SLASH:
      return "status-protocol-t2";
    case Parser::STATUS_PROTOCOL_VERSION_MAJOR:
      return "status-protocol-version-major";
    case Parser::STATUS_PROTOCOL_VERSION_MINOR:
      return "status-protocol-version-minor";
    case Parser::STATUS_CODE_BEGIN:
      return "status-code-begin";
    case Parser::STATUS_CODE:
      return "status-code";
    case Parser::STATUS_MESSAGE_BEGIN:
      return "status-message-begin";
    case Parser::STATUS_MESSAGE:
      return "status-message";
    case Parser::STATUS_MESSAGE_LF:
      return "status-message-lf";

    // message header
    case Parser::HEADER_NAME_BEGIN:
      return "header-name-begin";
    case Parser::HEADER_NAME:
      return "header-name";
    case Parser::HEADER_COLON:
      return "header-colon";
    case Parser::HEADER_VALUE_BEGIN:
      return "header-value-begin";
    case Parser::HEADER_VALUE:
      return "header-value";
    case Parser::HEADER_VALUE_LF:
      return "header-value-lf";
    case Parser::HEADER_VALUE_END:
      return "header-value-end";
    case Parser::HEADER_END_LF:
      return "header-end-lf";

    // LWS
    case Parser::LWS_BEGIN:
      return "lws-begin";
    case Parser::LWS_LF:
      return "lws-lf";
    case Parser::LWS_SP_HT_BEGIN:
      return "lws-sp-ht-begin";
    case Parser::LWS_SP_HT:
      return "lws-sp-ht";

    // message content
    case Parser::CONTENT_BEGIN:
      return "content-begin";
    case Parser::CONTENT:
      return "content";
    case Parser::CONTENT_ENDLESS:
      return "content-endless";
    case Parser::CONTENT_CHUNK_SIZE_BEGIN:
      return "content-chunk-size-begin";
    case Parser::CONTENT_CHUNK_SIZE:
      return "content-chunk-size";
    case Parser::CONTENT_CHUNK_LF1:
      return "content-chunk-lf1";
    case Parser::CONTENT_CHUNK_BODY:
      return "content-chunk-body";
    case Parser::CONTENT_CHUNK_LF2:
      return "content-chunk-lf2";
    case Parser::CONTENT_CHUNK_CR3:
      return "content-chunk-cr3";
    case Parser::CONTENT_CHUNK_LF3:
      return "content-chunk_lf3";
  }

  return "UNKNOWN";
}

inline bool Parser::isProcessingHeader() const {
  // XXX should we include request-line and status-line here, too?
  switch (state_) {
    case HEADER_NAME_BEGIN:
    case HEADER_NAME:
    case HEADER_COLON:
    case HEADER_VALUE_BEGIN:
    case HEADER_VALUE:
    case HEADER_VALUE_LF:
    case HEADER_VALUE_END:
    case HEADER_END_LF:
      return true;
    default:
      return false;
  }
}

inline bool Parser::isProcessingBody() const {
  switch (state_) {
    case CONTENT_BEGIN:
    case CONTENT:
    case CONTENT_ENDLESS:
    case CONTENT_CHUNK_SIZE_BEGIN:
    case CONTENT_CHUNK_SIZE:
    case CONTENT_CHUNK_LF1:
    case CONTENT_CHUNK_BODY:
    case CONTENT_CHUNK_LF2:
    case CONTENT_CHUNK_CR3:
    case CONTENT_CHUNK_LF3:
      return true;
    default:
      return false;
  }
}

Parser::Parser(ParseMode mode, HttpListener* listener)
    : mode_(mode),
      listener_(listener),
      state_(MESSAGE_BEGIN),
      bytesReceived_(0),
      lwsNext_(),
      lwsNull_(),
      method_(),
      entity_(),
      versionMajor_(),
      versionMinor_(),
      code_(0),
      message_(),
      name_(),
      value_(),
      chunked_(false),
      contentLength_(-1) {
  //.
}

std::size_t Parser::parseFragment(const BufferRef& chunk) {
  /*
   * CR               = 0x0D
   * LF               = 0x0A
   * SP               = 0x20
   * HT               = 0x09
   *
   * CRLF             = CR LF
   * LWS              = [CRLF] 1*( SP | HT )
   *
   * HTTP-message     = Request | Response
   *
   * generic-message  = start-line
   *                    *(message-header CRLF)
   *                    CRLF
   *                    [ message-body ]
   *
   * start-line       = Request-Line | Status-Line
   *
   * Request-Line     = Method SP Request-URI SP HTTP-Version CRLF
   *
   * Method           = "OPTIONS" | "GET" | "HEAD"
   *                  | "POST"    | "PUT" | "DELETE"
   *                  | "TRACE"   | "CONNECT"
   *                  | extension-method
   *
   * Request-URI      = "*" | absoluteURI | abs_path | authority
   *
   * extension-method = token
   *
   * Status-Line      = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
   *
   * HTTP-Version     = "HTTP" "/" 1*DIGIT "." 1*DIGIT
   * Status-Code      = 3*DIGIT
   * Reason-Phrase    = *<TEXT, excluding CR, LF>
   *
   * absoluteURI      = "http://" [user ':' pass '@'] hostname [abs_path] [qury]
   * abs_path         = "/" *CHAR
   * authority        = ...
   * token            = 1*<any CHAR except CTLs or seperators>
   * separator        = "(" | ")" | "<" | ">" | "@"
   *                  | "," | ";" | ":" | "\" | <">
   *                  | "/" | "[" | "]" | "?" | "="
   *                  | "{" | "}" | SP | HT
   *
   * message-header   = field-name ":" [ field-value ]
   * field-name       = token
   * field-value      = *( field-content | LWS )
   * field-content    = <the OCTETs making up the field-value
   *                    and consisting of either *TEXT or combinations
   *                    of token, separators, and quoted-string>
   *
   * message-body     = entity-body
   *                  | <entity-body encoded as per Transfer-Encoding>
   */

  const char* i = chunk.cbegin();
  const char* e = chunk.cend();

  const size_t initialOutOffset = 0;
  size_t result = initialOutOffset;
  size_t* nparsed = &result;

  auto nextChar = [&](size_t n = 1) {
    i += n;
    *nparsed += n;
    bytesReceived_ += n;
  };

  // TRACE(2, "process(curState:%s): size: %ld: '%s'", to_string(state()).c_str(),
  // chunk.size(), chunk.str().c_str());
  TRACE("process(curState:%s): size: %ld", to_string(state()).c_str(),
        chunk.size());

#if 0
    switch (state_) {
        case CONTENT: // fixed size content
            if (!passContent(chunk, nparsed))
                goto done;

            i += *nparsed;
            break;
        case CONTENT_ENDLESS: // endless-sized content (until stream end)
        {
            *nparsed += chunk.size();
            onMessageContent(chunk);
            goto done;
        }
        default:
            break;
    }
#endif

  while (i != e) {
#if 0 // !defined(NDEBUG)
    if (std::isprint(*i)) {
      TRACE("parse: %4ld, 0x%02X (%c),  %s", *nparsed, *i, *i,
            to_string(state()).c_str());
    } else {
      TRACE("parse: %4ld, 0x%02X,     %s", *nparsed, *i,
            to_string(state()).c_str());
    }
#endif

    switch (state_) {
      case MESSAGE_BEGIN:
        contentLength_ = -1;
        switch (mode_) {
          case REQUEST:
            state_ = REQUEST_LINE_BEGIN;
            versionMajor_ = 0;
            versionMinor_ = 0;
            break;
          case RESPONSE:
            state_ = STATUS_LINE_BEGIN;
            code_ = 0;
            versionMajor_ = 0;
            versionMinor_ = 0;
            break;
          case MESSAGE:
            state_ = HEADER_NAME_BEGIN;

            // an internet message has no special top-line,
            // so we just invoke the callback right away
            onMessageBegin();

            break;
        }
        break;
      case REQUEST_LINE_BEGIN:
        if (isToken(*i)) {
          state_ = REQUEST_METHOD;
          method_ = chunk.ref(*nparsed - initialOutOffset, 1);
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case REQUEST_METHOD:
        if (*i == SP) {
          state_ = REQUEST_ENTITY_BEGIN;
          nextChar();
        } else if (isToken(*i)) {
          method_.shr();
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case REQUEST_ENTITY_BEGIN:
        if (std::isprint(*i)) {
          entity_ = chunk.ref(*nparsed - initialOutOffset, 1);
          state_ = REQUEST_ENTITY;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case REQUEST_ENTITY:
        if (*i == SP) {
          state_ = REQUEST_PROTOCOL_BEGIN;
          nextChar();
        } else if (std::isprint(*i)) {
          entity_.shr();
          nextChar();
        } else if (*i == CR) {
          state_ = REQUEST_0_9_LF;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case REQUEST_0_9_LF:
        if (*i == LF) {
          onMessageBegin(method_, entity_, 0, 9);
          onMessageHeaderEnd();
          onMessageEnd();
          goto done;
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case REQUEST_PROTOCOL_BEGIN:
        if (*i != 'H') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = REQUEST_PROTOCOL_T1;
          nextChar();
        }
        break;
      case REQUEST_PROTOCOL_T1:
        if (*i != 'T') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = REQUEST_PROTOCOL_T2;
          nextChar();
        }
        break;
      case REQUEST_PROTOCOL_T2:
        if (*i != 'T') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = REQUEST_PROTOCOL_P;
          nextChar();
        }
        break;
      case REQUEST_PROTOCOL_P:
        if (*i != 'P') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = REQUEST_PROTOCOL_SLASH;
          nextChar();
        }
        break;
      case REQUEST_PROTOCOL_SLASH:
        if (*i != '/') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = REQUEST_PROTOCOL_VERSION_MAJOR;
          nextChar();
        }
        break;
      case REQUEST_PROTOCOL_VERSION_MAJOR:
        if (*i == '.') {
          state_ = REQUEST_PROTOCOL_VERSION_MINOR;
          nextChar();
        } else if (!std::isdigit(*i)) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          versionMajor_ = versionMajor_ * 10 + *i - '0';
          nextChar();
        }
        break;
      case REQUEST_PROTOCOL_VERSION_MINOR:
        if (*i == CR) {
          state_ = REQUEST_LINE_LF;
          nextChar();
        }
        else if (!std::isdigit(*i)) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          versionMinor_ = versionMinor_ * 10 + *i - '0';
          nextChar();
        }
        break;
      case REQUEST_LINE_LF:
        if (*i == LF) {
          state_ = HEADER_NAME_BEGIN;
          nextChar();

          TRACE("request-line: method=%s, entity=%s, vmaj=%d, vmin=%d",
                method_.str().c_str(), entity_.str().c_str(), versionMajor_,
                versionMinor_);

          onMessageBegin(method_, entity_, versionMajor_, versionMinor_);
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case STATUS_LINE_BEGIN:
      case STATUS_PROTOCOL_BEGIN:
        if (*i != 'H') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = STATUS_PROTOCOL_T1;
          nextChar();
        }
        break;
      case STATUS_PROTOCOL_T1:
        if (*i != 'T') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = STATUS_PROTOCOL_T2;
          nextChar();
        }
        break;
      case STATUS_PROTOCOL_T2:
        if (*i != 'T') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = STATUS_PROTOCOL_P;
          nextChar();
        }
        break;
      case STATUS_PROTOCOL_P:
        if (*i != 'P') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = STATUS_PROTOCOL_SLASH;
          nextChar();
        }
        break;
      case STATUS_PROTOCOL_SLASH:
        if (*i != '/') {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = STATUS_PROTOCOL_VERSION_MAJOR;
          nextChar();
        }
        break;
      case STATUS_PROTOCOL_VERSION_MAJOR:
        if (*i == '.') {
          state_ = STATUS_PROTOCOL_VERSION_MINOR;
          nextChar();
        } else if (!std::isdigit(*i)) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          versionMajor_ = versionMajor_ * 10 + *i - '0';
          nextChar();
        }
        break;
      case STATUS_PROTOCOL_VERSION_MINOR:
        if (*i == SP) {
          state_ = STATUS_CODE_BEGIN;
          nextChar();
        } else if (!std::isdigit(*i)) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          versionMinor_ = versionMinor_ * 10 + *i - '0';
          nextChar();
        }
        break;
      case STATUS_CODE_BEGIN:
        if (!std::isdigit(*i)) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
          break;
        }
        state_ = STATUS_CODE;
      /* fall through */
      case STATUS_CODE:
        if (std::isdigit(*i)) {
          code_ = code_ * 10 + *i - '0';
          nextChar();
        } else if (*i == SP) {
          state_ = STATUS_MESSAGE_BEGIN;
          nextChar();
        } else if (*i == CR) {  // no Status-Message passed
          state_ = STATUS_MESSAGE_LF;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case STATUS_MESSAGE_BEGIN:
        if (isText(*i)) {
          state_ = STATUS_MESSAGE;
          message_ = chunk.ref(*nparsed - initialOutOffset, 1);
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case STATUS_MESSAGE:
        if (isText(*i) && *i != CR && *i != LF) {
          message_.shr();
          nextChar();
        } else if (*i == CR) {
          state_ = STATUS_MESSAGE_LF;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case STATUS_MESSAGE_LF:
        if (*i == LF) {
          state_ = HEADER_NAME_BEGIN;
          nextChar();

          // TRACE("status-line: HTTP/%d.%d, code=%d, message=%s",
          // versionMajor_, versionMinor_, code_, message_.str().c_str());
          onMessageBegin(versionMajor_, versionMinor_, code_, message_);
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case HEADER_NAME_BEGIN:
        if (isToken(*i)) {
          name_ = chunk.ref(*nparsed - initialOutOffset, 1);
          state_ = HEADER_NAME;
          nextChar();
        } else if (*i == CR) {
          state_ = HEADER_END_LF;
          nextChar();
        }
        else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case HEADER_NAME:
        if (isToken(*i)) {
          name_.shr();
          nextChar();
        } else if (*i == ':') {
          state_ = LWS_BEGIN;
          lwsNext_ = HEADER_VALUE_BEGIN;
          lwsNull_ = HEADER_VALUE_END;  // only (CR LF) parsed, assume empty
                                        // value & go on with next header
          nextChar();
        } else if (*i == CR) {
          state_ = LWS_LF;
          lwsNext_ = HEADER_COLON;
          lwsNull_ = PROTOCOL_ERROR;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case HEADER_COLON:
        if (*i == ':') {
          state_ = LWS_BEGIN;
          lwsNext_ = HEADER_VALUE_BEGIN;
          lwsNull_ = HEADER_VALUE_END;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case LWS_BEGIN:
        if (*i == CR) {
          state_ = LWS_LF;
          nextChar();
        } else if (*i == SP || *i == HT) {
          state_ = LWS_SP_HT;
          nextChar();
        } else if (std::isprint(*i)) {
          state_ = lwsNext_;
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case LWS_LF:
        if (*i == LF) {
          state_ = LWS_SP_HT_BEGIN;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case LWS_SP_HT_BEGIN:
        if (*i == SP || *i == HT) {
          if (!value_.empty()) value_.shr(3);  // CR LF (SP | HT)

          state_ = LWS_SP_HT;
          nextChar();
        } else {
          // only (CF LF) parsed so far and no 1*(SP | HT) found.
          if (lwsNull_ == PROTOCOL_ERROR) {
            onProtocolError(HttpStatus::BadRequest);
          }
          state_ = lwsNull_;
          // XXX no nparsed/i-update
        }
        break;
      case LWS_SP_HT:
        if (*i == SP || *i == HT) {
          if (!value_.empty())
            value_.shr();

          nextChar();
        } else
          state_ = lwsNext_;
        break;
      case HEADER_VALUE_BEGIN:
        if (isText(*i)) {
          value_ = chunk.ref(*nparsed - initialOutOffset, 1);
          nextChar();
          state_ = HEADER_VALUE;
        } else if (*i == CR) {
          state_ = HEADER_VALUE_LF;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case HEADER_VALUE:
        if (*i == CR) {
          state_ = LWS_LF;
          lwsNext_ = HEADER_VALUE;
          lwsNull_ = HEADER_VALUE_END;
          nextChar();
        }
        else if (isText(*i)) {
          value_.shr();
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case HEADER_VALUE_LF:
        if (*i == LF) {
          state_ = HEADER_VALUE_END;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case HEADER_VALUE_END: {
        TRACE("header: name='%s', value='%s'", name_.str().c_str(),
              value_.str().c_str());

        if (iequals(name_, "Content-Length")) {
          contentLength_ = value_.toInt();
          TRACE("set content length to: %ld", contentLength_);
          onMessageHeader(name_, value_);
        } else if (iequals(name_, "Transfer-Encoding")) {
          if (iequals(value_, "chunked")) {
            chunked_ = true;
            // do not pass header to the upper layer if we've
            // processed it
          } else {
            onMessageHeader(name_, value_);
          }
        } else {
          onMessageHeader(name_, value_);
        }

        name_.clear();
        value_.clear();

        // continue with the next header
        state_ = HEADER_NAME_BEGIN;

        break;
      }
      case HEADER_END_LF:
        if (*i == LF) {
          if (isContentExpected())
            state_ = CONTENT_BEGIN;
          else
            state_ = MESSAGE_BEGIN;

          nextChar();

          onMessageHeaderEnd();

          if (!isContentExpected()) {
            onMessageEnd();
            goto done;
          }
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case CONTENT_BEGIN:
        if (chunked_)
          state_ = CONTENT_CHUNK_SIZE_BEGIN;
        else if (contentLength_ >= 0)
          state_ = CONTENT;
        else
          state_ = CONTENT_ENDLESS;
        break;
      case CONTENT_ENDLESS: {
        // body w/o content-length (allowed in simple MESSAGE types only)
        BufferRef c(chunk.ref(*nparsed - initialOutOffset));

        // TRACE("prepared content-chunk (%ld bytes): %s", c.size(),
        // c.str().c_str());

        nextChar(c.size());

        onMessageContent(c);

        break;
      }
      case CONTENT: {
        // fixed size content length
        std::size_t offset = *nparsed - initialOutOffset;
        std::size_t chunkSize = std::min(static_cast<size_t>(contentLength_),
                                         chunk.size() - offset);

        contentLength_ -= chunkSize;
        nextChar(chunkSize);

        onMessageContent(chunk.ref(offset, chunkSize));

        if (contentLength_ == 0)
          state_ = MESSAGE_BEGIN;

        if (state_ == MESSAGE_BEGIN) {
          onMessageEnd();
          goto done;
        }

        break;
      }
      case CONTENT_CHUNK_SIZE_BEGIN:
        if (!std::isxdigit(*i)) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
          break;
        }
        state_ = CONTENT_CHUNK_SIZE;
        contentLength_ = 0;
      /* fall through */
      case CONTENT_CHUNK_SIZE:
        if (*i == CR) {
          state_ = CONTENT_CHUNK_LF1;
          nextChar();
        } else if (*i >= '0' && *i <= '9') {
          contentLength_ = contentLength_ * 16 + *i - '0';
          nextChar();
        } else if (*i >= 'a' && *i <= 'f') {
          contentLength_ = contentLength_ * 16 + 10 + *i - 'a';
          nextChar();
        } else if (*i >= 'A' && *i <= 'F') {
          contentLength_ = contentLength_ * 16 + 10 + *i - 'A';
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case CONTENT_CHUNK_LF1:
        if (*i != LF) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          // TRACE("content_length: %ld", contentLength_);
          if (contentLength_ != 0)
            state_ = CONTENT_CHUNK_BODY;
          else
            state_ = CONTENT_CHUNK_CR3;

          nextChar();
        }
        break;
      case CONTENT_CHUNK_BODY:
        if (contentLength_) {
          std::size_t offset = *nparsed - initialOutOffset;
          std::size_t chunkSize = std::min(static_cast<size_t>(contentLength_),
                                           chunk.size() - offset);
          contentLength_ -= chunkSize;
          nextChar(chunkSize);

          onMessageContent(chunk.ref(offset, chunkSize));
        } else if (*i == CR) {
          state_ = CONTENT_CHUNK_LF2;
          nextChar();
        } else {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        }
        break;
      case CONTENT_CHUNK_LF2:
        if (*i != LF) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = CONTENT_CHUNK_SIZE;
          nextChar();
        }
        break;
      case CONTENT_CHUNK_CR3:
        if (*i != CR) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          state_ = CONTENT_CHUNK_LF3;
          nextChar();
        }
        break;
      case CONTENT_CHUNK_LF3:
        if (*i != LF) {
          onProtocolError(HttpStatus::BadRequest);
          state_ = PROTOCOL_ERROR;
        } else {
          nextChar();

          state_ = MESSAGE_BEGIN;

          onMessageEnd();
          goto done;
        }
        break;
      case PROTOCOL_ERROR:
        goto done;
      default:
#if !defined(NDEBUG)
        TRACE("parse: unknown state %i", state_);
        if (std::isprint(*i)) {
          TRACE("parse: internal error at nparsed: %ld, character: '%c'",
                *nparsed, *i);
        } else {
          TRACE("parse: internal error at nparsed: %ld, character: 0x%02X",
                *nparsed, *i);
        }
        TRACE("%s", chunk.hexdump().c_str());
#endif
        goto done;
    }
  }
  // we've reached the end of the chunk

  if (state_ == CONTENT_BEGIN) {
    // we've just parsed all headers but no body yet.

    if (contentLength_ < 0 && !chunked_ && mode_ != MESSAGE) {
      // and there's no body to come

      // subsequent calls to process() parse next request(s).
      state_ = MESSAGE_BEGIN;

      onMessageEnd();
      goto done;
    }
  }

done:
  return *nparsed - initialOutOffset;
}

void Parser::reset() {
  state_ = MESSAGE_BEGIN;
  bytesReceived_ = 0;
}

inline bool Parser::isChar(char value) {
  return static_cast<unsigned>(value) <= 127;
}

inline bool Parser::isControl(char value) {
  return (value >= 0 && value <= 31) || value == 127;
}

inline bool Parser::isSeparator(char value) {
  switch (value) {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case SP:
    case HT:
      return true;
    default:
      return false;
  }
}

inline bool Parser::isToken(char value) {
  return isChar(value) && !(isControl(value) || isSeparator(value));
}

inline bool Parser::isText(char value) {
  // TEXT = <any OCTET except CTLs but including LWS>
  return !isControl(value) || value == SP || value == HT;
}

inline HttpVersion make_version(int versionMajor, int versionMinor) {
  if (versionMajor == 0) {
    if (versionMinor == 9)
      return HttpVersion::VERSION_0_9;
    else
      return HttpVersion::UNKNOWN;
  }

  if (versionMajor == 1) {
    if (versionMinor == 0)
      return HttpVersion::VERSION_1_0;
    else if (versionMinor == 1)
      return HttpVersion::VERSION_1_1;
  }

  return HttpVersion::UNKNOWN;
}

void Parser::onMessageBegin(const BufferRef& method,
                            const BufferRef& entity,
                            int versionMajor,
                            int versionMinor) {
  HttpVersion version = make_version(versionMajor, versionMinor);

  if (version == HttpVersion::UNKNOWN)
    RAISE_HTTP(HttpVersionNotSupported);

  if (listener_)
    listener_->onMessageBegin(method, entity, version);
}

void Parser::onMessageBegin(int versionMajor, int versionMinor,
                                int status, const BufferRef& text) {
  HttpVersion version = make_version(versionMajor, versionMinor);
  if (version == HttpVersion::UNKNOWN)
    RAISE_HTTP(HttpVersionNotSupported);

  if (!listener_)
    return;

  listener_->onMessageBegin(version, static_cast<HttpStatus>(code_), text);
}

void Parser::onMessageBegin() {
  if (listener_)
    listener_->onMessageBegin();
}

void Parser::onMessageHeader(const BufferRef& name,
                                        const BufferRef& value) {
  if (listener_)
    listener_->onMessageHeader(name, value);
}

void Parser::onMessageHeaderEnd() {
  if (listener_)
    listener_->onMessageHeaderEnd();
}

void Parser::onMessageContent(const BufferRef& chunk) {
  if (listener_)
    listener_->onMessageContent(chunk);
}

void Parser::onMessageEnd() {
  if (listener_)
    listener_->onMessageEnd();
}

void Parser::onProtocolError(HttpStatus code, const std::string& message) {
  if (listener_) {
    listener_->onProtocolError(code, message);
  }
}

}  // namespace http1
}  // namespace http
}  // namespace cortex
