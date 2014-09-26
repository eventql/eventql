#include <xzero/net/LocalConnector.h>
#include <xzero/net/ConnectionFactory.h>
#include <xzero/net/Connection.h>
#include <xzero/WallClock.h>
#include <xzero/logging/LogSource.h>
#include <xzero/executor/Executor.h>
#include <algorithm>

namespace xzero {

static LogSource localConnectorLogger("net.LocalConnector");
#ifndef NDEBUG
#define TRACE(msg...) do { localConnectorLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

// {{{ LocalEndPoint impl
LocalEndPoint::LocalEndPoint(LocalConnector* connector)
    : ByteArrayEndPoint(connector), connector_(connector) {}

LocalEndPoint::~LocalEndPoint() {
}

void LocalEndPoint::close() {
  ByteArrayEndPoint::close();
  connector_->onEndPointClosed(this);
}
// }}}

LocalConnector::LocalConnector(Executor* executor)
    : Connector("local", executor, WallClock::system()),
      isStarted_(false),
      pendingConnects_(),
      connectedEndPoints_() {
}

LocalConnector::~LocalConnector() {
  //.
}

void LocalConnector::start() {
  isStarted_ = true;
}

void LocalConnector::stop() {
  isStarted_ = false;
}

std::list<EndPoint*> LocalConnector::connectedEndPoints() {
  std::list<EndPoint*> result;
  for (std::shared_ptr<LocalEndPoint>& ep : connectedEndPoints_) {
    result.push_back(ep.get());
  }
  return result;
}

std::shared_ptr<LocalEndPoint> LocalConnector::createClient(
    const std::string& rawRequestMessage) {

  assert(isStarted());

  pendingConnects_.emplace_back(new LocalEndPoint(this));
  std::shared_ptr<LocalEndPoint> endpoint = pendingConnects_.back();
  endpoint->setInput(rawRequestMessage);

  executor()->execute([this]() { acceptOne(); });

  return endpoint;
}

bool LocalConnector::acceptOne() {
  assert(isStarted());

  if (pendingConnects_.empty()) {
    return false;
  }

  std::shared_ptr<LocalEndPoint> endpoint = pendingConnects_.front();
  pendingConnects_.pop_front();
  connectedEndPoints_.push_back(endpoint);

  auto connection = defaultConnectionFactory()->create(this, endpoint);
  connection->onOpen();

  return true;
}

void LocalConnector::onEndPointClosed(LocalEndPoint* endpoint) {
  TRACE("%p onEndPointClosed: connection=%p, endpoint=%p", this,
        endpoint->connection(), endpoint);

  // try connected endpoints
  auto i = std::find_if(connectedEndPoints_.begin(), connectedEndPoints_.end(),
                        [endpoint](const std::shared_ptr<LocalEndPoint>& ep) {
    return endpoint == ep.get();
  });

  if (i != connectedEndPoints_.end()) {
    endpoint->connection()->onClose();
    connectedEndPoints_.erase(i);
    if (!endpoint->isBusy()) {
      // do only actually delete if not currently inside an io handler
      release(endpoint->connection());
    }
    return;
  }

  // try pending endpoints
  auto k = std::find_if(pendingConnects_.begin(), pendingConnects_.end(),
                        [endpoint](const std::shared_ptr<LocalEndPoint>& ep) {
    return ep.get() == endpoint;
  });

  if (k != pendingConnects_.end()) {
    pendingConnects_.erase(k);
    release(endpoint->connection());
  }
}

void LocalConnector::release(Connection* connection) {
  assert(connection != nullptr);
  assert(dynamic_cast<LocalEndPoint*>(connection->endpoint()) != nullptr);
  assert(!static_cast<LocalEndPoint*>(connection->endpoint())->isBusy());

  TRACE("%p release: connection=%p, endpoint=%p", this, connection,
        connection->endpoint());

  delete connection;
}

}  // namespace xzero
