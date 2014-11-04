// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/v1/Http1ConnectionFactory.h>
#include <xzero/http/v1/HttpConnection.h>
#include <xzero/net/Connector.h>

namespace xzero {
namespace http1 {

Http1ConnectionFactory::Http1ConnectionFactory(
    WallClock* clock,
    size_t maxRequestUriLength,
    size_t maxRequestBodyLength,
    size_t maxRequestCount,
    TimeSpan maxKeepAlive)
    : HttpConnectionFactory("http", clock, maxRequestUriLength,
                            maxRequestBodyLength),
      maxRequestCount_(maxRequestCount),
      maxKeepAlive_(maxKeepAlive) {
  setInputBufferSize(16 * 1024);
}

Http1ConnectionFactory::~Http1ConnectionFactory() {
}

Connection* Http1ConnectionFactory::create(Connector* connector,
                                           std::shared_ptr<EndPoint> endpoint) {
  return configure(new http1::HttpConnection(endpoint,
                                             connector->executor(),
                                             handler(),
                                             dateGenerator(),
                                             outputCompressor(),
                                             maxRequestUriLength(),
                                             maxRequestBodyLength(),
                                             maxRequestCount(),
                                             maxKeepAlive()),
                   connector);
}

}  // namespace http1
}  // namespace xzero
