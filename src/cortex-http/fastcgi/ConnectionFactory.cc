// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/fastcgi/ConnectionFactory.h>
#include <cortex-http/fastcgi/Connection.h>
#include <cortex-base/net/Connector.h>
#include <cortex-base/WallClock.h>

namespace cortex {
namespace http {
namespace fastcgi {

ConnectionFactory::ConnectionFactory()
    : ConnectionFactory(WallClock::system(),
                        4096,
                        4 * 1024 * 1024,
                        TimeSpan::fromSeconds(8)) {
}

ConnectionFactory::ConnectionFactory(
    WallClock* clock,
    size_t maxRequestUriLength,
    size_t maxRequestBodyLength,
    TimeSpan maxKeepAlive)
    : HttpConnectionFactory("fastcgi",
                            clock,
                            maxRequestUriLength,
                            maxRequestBodyLength),
      maxKeepAlive_(maxKeepAlive) {
  setInputBufferSize(16 * 1024);
}

ConnectionFactory::~ConnectionFactory() {
}

cortex::Connection* ConnectionFactory::create(
    Connector* connector,
    EndPoint* endpoint) {
  return configure(new Connection(endpoint,
                                  connector->executor(),
                                  handler(),
                                  dateGenerator(),
                                  outputCompressor(),
                                  maxRequestUriLength(),
                                  maxRequestBodyLength(),
                                  maxKeepAlive()),
                   connector);
}

} // namespace fastcgi
} // namespace http
} // namespace cortex
