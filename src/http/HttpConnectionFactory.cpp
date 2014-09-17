#include <xzero/http/HttpConnectionFactory.h>
#include <xzero/net/Connection.h>
#include <xzero/WallClock.h>

namespace xzero {

HttpConnectionFactory::HttpConnectionFactory(const std::string& protocolName)
    : ConnectionFactory(protocolName),
      clock_(nullptr),
      dateGenerator_(nullptr) {
  //.
}

HttpConnectionFactory::~HttpConnectionFactory() {
  //.
}

void HttpConnectionFactory::setHandler(HttpHandler&& handler) {
  handler_ = std::move(handler);
}

void HttpConnectionFactory::setClock(WallClock* clock) {
  if (clock != nullptr) {
    dateGenerator_.reset(new HttpDateGenerator(clock));
  } else {
    dateGenerator_.reset(nullptr);
  }
  clock_ = clock;
}

Connection* HttpConnectionFactory::configure(Connection* connection,
                                             Connector* connector) {
  if (clock_ != nullptr) {
    connection->setCreationTime(clock_->get());
  }

  return ConnectionFactory::configure(connection, connector);
}

}  // namespace xzero
