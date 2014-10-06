#include <xzero/http/HttpConnectionFactory.h>
#include <xzero/net/Connection.h>
#include <xzero/WallClock.h>

namespace xzero {

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

}  // namespace xzero
