// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/HttpConnectionFactory.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/WallClock.h>

namespace cortex {
namespace http {

HttpConnectionFactory::HttpConnectionFactory(
    const std::string& protocolName,
    WallClock* clock,
    size_t maxRequestUriLength,
    size_t maxRequestBodyLength)
    : ConnectionFactory(protocolName),
      maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      outputCompressor_(new HttpOutputCompressor()),
      dateGenerator_(clock ? new HttpDateGenerator(clock) : nullptr) {
  //.
}

HttpConnectionFactory::~HttpConnectionFactory() {
  //.
}

void HttpConnectionFactory::setHandler(HttpHandler&& handler) {
  handler_ = std::move(handler);
}

Connection* HttpConnectionFactory::configure(Connection* connection,
                                             Connector* connector) {
  return ConnectionFactory::configure(connection, connector);
}

}  // namespace http
}  // namespace cortex
