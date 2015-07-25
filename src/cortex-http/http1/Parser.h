// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/Buffer.h>
#include <cortex-http/HttpStatus.h>
#include <memory>

namespace cortex {
namespace http {

class HttpListener;

namespace http1 {

/**
 * HTTP/1.1 message parser.
 *
 * This API parses an HTTP/1 message. No semantic checks are performed.
 *
 * @see HttpListener
 */
class CORTEX_HTTP_API Parser {
 public:
  // {{{ enums
  /**
   * defines whether to parse HTTP requests, responses, or plain messages.
   */
  enum ParseMode {
    /** the message to parse does not contain either an HTTP request-line nor
      * response status-line but headers and a body. */
    MESSAGE,
    /** the message to parse is an HTTP request. */
    REQUEST,
    /** the message to parse is an HTTP response. */
    RESPONSE,
  };

  /**
   * defines list of states the parser is in while processing the message.
   */
  enum State {
    // artificial
    PROTOCOL_ERROR = 1,
    MESSAGE_BEGIN,

    // Request-Line
    REQUEST_LINE_BEGIN = 100,
    REQUEST_METHOD,
    REQUEST_ENTITY_BEGIN,
    REQUEST_ENTITY,
    REQUEST_PROTOCOL_BEGIN,
    REQUEST_PROTOCOL_T1,
    REQUEST_PROTOCOL_T2,
    REQUEST_PROTOCOL_P,
    REQUEST_PROTOCOL_SLASH,
    REQUEST_PROTOCOL_VERSION_MAJOR,
    REQUEST_PROTOCOL_VERSION_MINOR,
    REQUEST_LINE_LF,
    REQUEST_0_9_LF,

    // Status-Line
    STATUS_LINE_BEGIN = 150,
    STATUS_PROTOCOL_BEGIN,
    STATUS_PROTOCOL_T1,
    STATUS_PROTOCOL_T2,
    STATUS_PROTOCOL_P,
    STATUS_PROTOCOL_SLASH,
    STATUS_PROTOCOL_VERSION_MAJOR,
    STATUS_PROTOCOL_VERSION_MINOR,
    STATUS_CODE_BEGIN,
    STATUS_CODE,
    STATUS_MESSAGE_BEGIN,
    STATUS_MESSAGE,
    STATUS_MESSAGE_LF,

    // message-headers
    HEADER_NAME_BEGIN = 200,
    HEADER_NAME,
    HEADER_COLON,
    HEADER_VALUE_BEGIN,
    HEADER_VALUE,
    HEADER_VALUE_LF,
    HEADER_VALUE_END,
    HEADER_END_LF,

    // LWS ::= [CR LF] 1*(SP | HT)
    LWS_BEGIN = 300,
    LWS_LF,
    LWS_SP_HT_BEGIN,
    LWS_SP_HT,

    // message-content
    CONTENT_BEGIN = 400,
    CONTENT,
    CONTENT_ENDLESS = 405,
    CONTENT_CHUNK_SIZE_BEGIN = 410,
    CONTENT_CHUNK_SIZE,
    CONTENT_CHUNK_LF1,
    CONTENT_CHUNK_BODY,
    CONTENT_CHUNK_LF2,
    CONTENT_CHUNK_CR3,
    CONTENT_CHUNK_LF3
  };
  // }}}

  /**
   * Initializes the HTTP/1.1 message processor.
   *
   * @param mode REQUEST: parses and processes an HTTP/1.1 Request,
   *             RESPONSE: parses and processes an HTTP/1.1 Response.
   *             MESSAGE: parses and processes an HTTP/1.1 message, that is,
   *             without the first request/status line - just headers and
   *             content.
   *
   * @param listener an HttpListener for receiving HTTP message events.
   *
   * @note No member variable may be modified after the hook invokation
   *       returned with a false return code, which means, that processing
   *       is to be cancelled and thus, may imply, that the object itself
   *       may have been already deleted.
   */
  explicit Parser(ParseMode mode, HttpListener* listener = nullptr);

  State state() const { return state_; }

  /**
   * Processes a message-chunk.
   *
   * @param chunk the chunk of bytes to process
   * @return      number of bytes actually parsed and processed
   */
  size_t parseFragment(const BufferRef& chunk);

  ssize_t contentLength() const;
  bool isChunked() const { return chunked_; }

  void reset();

  bool isProcessingHeader() const;
  bool isProcessingBody() const;

  bool isContentExpected() const CORTEX_NOEXCEPT {
    return contentLength_ > 0 || chunked_ ||
           (contentLength_ < 0 && mode_ != REQUEST);
  }

  void setListener(HttpListener* listener) { listener_ = listener; }
  HttpListener* listener() const { return listener_; }

  size_t bytesReceived() const noexcept { return bytesReceived_; }

 private:
  static inline bool isChar(char value);
  static inline bool isControl(char value);
  static inline bool isSeparator(char value);
  static inline bool isToken(char value);
  static inline bool isText(char value);

  void onMessageBegin(const BufferRef& method, const BufferRef& entity,
                      int versionMajor, int versionMinor);
  void onMessageBegin(int versionMajor, int versionMinor, int code,
                      const BufferRef& text);
  void onMessageBegin();
  void onMessageHeader(const BufferRef& name, const BufferRef& value);
  void onMessageHeaderEnd();
  void onMessageContent(const BufferRef& chunk);
  void onMessageEnd();
  void onProtocolError(HttpStatus code, const std::string& message = "");

 private:
  // lexer constants
  enum { CR = 0x0D, LF = 0x0A, SP = 0x20, HT = 0x09 };

  ParseMode mode_;          //!< parsing mode (request/response/something)
  HttpListener* listener_;  //!< HTTP message component listener
  State state_;             //!< the current parser/processing state

  // stats
  size_t bytesReceived_;

  // implicit LWS handling
  State lwsNext_;  //!< state to apply on successfull LWS
  State lwsNull_;  //!< state to apply on (CR LF) but no 1*(SP | HT)

  // request-line
  BufferRef method_;  //!< HTTP request method
  BufferRef entity_;  //!< HTTP request entity
  int versionMajor_;  //!< HTTP request/response version major
  int versionMinor_;  //!< HTTP request/response version minor

  // status-line
  int code_;          //!< response status code
  BufferRef message_; //!< response status message

  // current parsed header
  BufferRef name_;
  BufferRef value_;

  // body
  bool chunked_;  //!< whether or not request content is chunked encoded
  ssize_t contentLength_;  //!< content length of whole content or current chunk
};

CORTEX_HTTP_API std::string to_string(Parser::State state);

}  // namespace http1
}  // namespace http
}  // namespace cortex
