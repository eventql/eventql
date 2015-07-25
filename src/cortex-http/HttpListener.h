// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/Buffer.h>
#include <cortex-http/HttpVersion.h>
#include <cortex-http/HttpStatus.h>
#include <memory>

namespace cortex {
namespace http {

/**
 * HTTP message observer.
 *
 * The interface methods get invoked by the @c HttpParser
 * to inform the observer about certain informations being parsed.
 *
 * @see HttpParser
 */
class CORTEX_HTTP_API HttpListener {
 public:
  virtual ~HttpListener() {}

  /** HTTP/1.1 Request-Line, that has been fully parsed.
   *
   * @param method the request-method (e.g. GET or POST)
   * @param entity the requested URI (e.g. /index.html)
   * @param version HTTP version (e.g. 0.9 or 2.0)
   */
  virtual void onMessageBegin(const BufferRef& method, const BufferRef& entity,
                              HttpVersion version);

  /** HTTP/1.1 response Status-Line, that has been fully parsed.
   *
   * @param version HTTP version (e.g. 0.9 or 2.0)
   * @param code HTTP response status code (e.g. 200 or 404)
   * @param text HTTP response status text (e.g. "Ok" or "Not Found")
   */
  virtual void onMessageBegin(HttpVersion version, HttpStatus code,
                              const BufferRef& text);

  /**
   * HTTP generic message begin (neither request nor response message).
   */
  virtual void onMessageBegin();

  /**
   * Single HTTP message header.
   *
   * @param name the header name
   * @param value the header value
   *
   * @note Does nothing but returns true by default.
   */
  virtual void onMessageHeader(const BufferRef& name, const BufferRef& value);

  /**
   * Invoked once all request headers have been fully parsed.
   *
   * (no possible content parsed yet)
   *
   * @note Does nothing but returns true by default.
   *
   * @retval true continue processing further content (if any)
   * @retval false abort message processing
   */
  virtual void onMessageHeaderEnd();

  /**
   * Invoked for every chunk of message content being processed.
   *
   * @note Does nothing but returns true by default.
   *
   * @retval true continue processing further content (if any)
   * @retval false abort message processing
   */
  virtual void onMessageContent(const BufferRef& chunk);

  /**
   * Invoked once a fully HTTP message has been processed.
   *
   * @note Does nothing but returns true by default.
   *
   * @retval true continue processing further content (if any)
   * @retval false abort message processing
   */
  virtual void onMessageEnd();

  /**
   * HTTP message transport protocol error.
   */
  virtual void onProtocolError(HttpStatus code, const std::string& message) = 0;
};

}  // namespace http
}  // namespace cortex
