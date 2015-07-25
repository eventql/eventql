// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/TimeSpan.h>
#include <cortex-http/HttpConnectionFactory.h>

namespace cortex {
namespace http {
namespace fastcgi {

/**
 * Connection factory for FastCGI connections.
 */
class CORTEX_HTTP_API ConnectionFactory : public HttpConnectionFactory {
 public:
  ConnectionFactory();

  ConnectionFactory(
      WallClock* clock,
      size_t maxRequestUriLength,
      size_t maxRequestBodyLength,
      TimeSpan maxKeepAlive);

  ~ConnectionFactory();

  TimeSpan maxKeepAlive() const CORTEX_NOEXCEPT { return maxKeepAlive_; }
  void setMaxKeepAlive(TimeSpan value) { maxKeepAlive_ = value; }

  cortex::Connection* create(Connector* connector, EndPoint* endpoint) override;

 private:
  TimeSpan maxKeepAlive_;
};

} // namespace fastcgi
} // namespace http
} // namespace cortex
