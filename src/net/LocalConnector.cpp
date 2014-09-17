#include <xzero/net/LocalConnector.h>
#include <xzero/net/ConnectionFactory.h>
#include <xzero/net/Connection.h>
#include <xzero/executor/Executor.h>
#include <algorithm>

namespace xzero {

// {{{ LocalEndPoint impl
LocalEndPoint::LocalEndPoint(LocalConnector* connector)
    : ByteArrayEndPoint(), connector_(connector) {}

LocalEndPoint::~LocalEndPoint() {}

void LocalEndPoint::close() {
  ByteArrayEndPoint::close();
  connector_->onEndPointClosed(this);
}
// }}}

LocalConnector::LocalConnector(Executor* executor)
    : Connector("local", executor),
      isStarted_(false),
      pendingConnects_(),
      connectedEndPoints_() {
}

LocalConnector::~LocalConnector() {
  //.
}

void LocalConnector::start() { isStarted_ = true; }

void LocalConnector::stop() { isStarted_ = false; }

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
  // try connected endpoints
  auto i = std::find_if(connectedEndPoints_.begin(), connectedEndPoints_.end(),
                        [endpoint](const std::shared_ptr<LocalEndPoint>& ep) {
    return endpoint == ep.get();
  });

  if (i != connectedEndPoints_.end()) {
    connectedEndPoints_.erase(i);
    delete endpoint->connection();
    return;
  }

  // try pending endpoints
  auto k = std::find_if(pendingConnects_.begin(), pendingConnects_.end(),
                        [endpoint](const std::shared_ptr<LocalEndPoint>& ep) {
    return ep.get() == endpoint;
  });

  if (k != pendingConnects_.end()) {
    pendingConnects_.erase(k);
    delete endpoint->connection();
    return;
  }
}

}  // namespace xzero
