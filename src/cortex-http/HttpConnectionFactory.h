// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/net/ConnectionFactory.h>
#include <cortex-http/HttpHandler.h>
#include <cortex-http/HttpDateGenerator.h>
#include <cortex-http/HttpOutputCompressor.h>
#include <memory>

namespace cortex {

class WallClock;

namespace http {

/**
 * Base HTTP connection factory.
 *
 * This provides common functionality to all HTTP connection factories.
 */
class CORTEX_HTTP_API HttpConnectionFactory : public ConnectionFactory {
 public:
  /**
   * Base initiailization for the HTTP connection factory.
   *
   * @param protocolName HTTP protocol name, e.g. http1, http2, ssl+http1, ...
   * @param clock the wallclock used for generating @c Date response header
   * @param maxRequestUriLength maximum number of bytes for the request URI
   * @param maxRequestBodyLength maximum number of bytes for the request body
   */
  HttpConnectionFactory(
      const std::string& protocolName,
      WallClock* clock,
      size_t maxRequestUriLength,
      size_t maxRequestBodyLength);

  ~HttpConnectionFactory();

  size_t maxRequestUriLength() const CORTEX_NOEXCEPT { return maxRequestUriLength_; }
  void setMaxRequestUriLength(size_t value) { maxRequestUriLength_ = value; }

  size_t maxRequestBodyLength() const CORTEX_NOEXCEPT { return maxRequestBodyLength_; }
  void setMaxRequestBodyLength(size_t value) { maxRequestBodyLength_ = value; }

  const HttpHandler& handler() const CORTEX_NOEXCEPT { return handler_; }
  void setHandler(HttpHandler&& handler);

  /** Access to the output compression service. */
  HttpOutputCompressor* outputCompressor() const CORTEX_NOEXCEPT;

  /** Access to the @c Date response header generator. */
  HttpDateGenerator* dateGenerator() const CORTEX_NOEXCEPT;

  Connection* configure(Connection* connection, Connector* connector) override;

 private:
  size_t maxRequestUriLength_;
  size_t maxRequestBodyLength_;
  HttpHandler handler_;
  std::unique_ptr<HttpOutputCompressor> outputCompressor_;
  std::unique_ptr<HttpDateGenerator> dateGenerator_;
};

// {{{ inlines
inline HttpOutputCompressor* HttpConnectionFactory::outputCompressor() const CORTEX_NOEXCEPT {
  return outputCompressor_.get();
}

inline HttpDateGenerator* HttpConnectionFactory::dateGenerator() const CORTEX_NOEXCEPT {
  return dateGenerator_.get();
}
// }}}

}  // namespace http
}  // namespace cortex
