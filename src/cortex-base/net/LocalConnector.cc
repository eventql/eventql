// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/LocalConnector.h>
#include <cortex-base/net/ConnectionFactory.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/logging.h>
#include <cortex-base/executor/Executor.h>
#include <algorithm>

namespace cortex {

#ifndef NDEBUG
#define TRACE(msg...) logTrace("net.LocalConnector", msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

// {{{ LocalEndPoint impl
LocalEndPoint::LocalEndPoint(LocalConnector* connector)
    : ByteArrayEndPoint(connector), connector_(connector) {}

LocalEndPoint::~LocalEndPoint() {
  TRACE("%p ~LocalEndPoint: connection=%p", this, connection());
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

std::list<RefPtr<EndPoint>> LocalConnector::connectedEndPoints() {
  std::list<RefPtr<EndPoint>> result;

  for (const auto& ep: connectedEndPoints_)
    result.push_back(ep.as<EndPoint>());

  return result;
}

RefPtr<LocalEndPoint> LocalConnector::createClient(
    const std::string& rawRequestMessage) {

  assert(isStarted());

  pendingConnects_.emplace_back(new LocalEndPoint(this));
  RefPtr<LocalEndPoint> endpoint = pendingConnects_.back();
  endpoint->setInput(rawRequestMessage);

  executor()->execute(std::bind((void (LocalConnector::*)()) &LocalConnector::acceptOne, this));

  return endpoint;
}

bool LocalConnector::acceptOne() {
  assert(isStarted());

  if (pendingConnects_.empty()) {
    return false;
  }

  RefPtr<LocalEndPoint> endpoint = pendingConnects_.front();
  pendingConnects_.pop_front();
  connectedEndPoints_.push_back(endpoint);

  auto connection = defaultConnectionFactory()->create(this, endpoint.get());
  connection->onOpen();

  return true;
}

void LocalConnector::onEndPointClosed(LocalEndPoint* endpoint) {
  TRACE("%p onEndPointClosed: connection=%p, endpoint=%p", this,
        endpoint->connection(), endpoint);

  // try connected endpoints
  auto i = std::find_if(connectedEndPoints_.begin(), connectedEndPoints_.end(),
                        [endpoint](const RefPtr<LocalEndPoint>& ep) {
    return endpoint == ep.get();
  });

  if (i != connectedEndPoints_.end()) {
    endpoint->connection()->onClose();
    connectedEndPoints_.erase(i);
    return;
  }

  // try pending endpoints
  auto k = std::find_if(pendingConnects_.begin(), pendingConnects_.end(),
                        [endpoint](const RefPtr<LocalEndPoint>& ep) {
    return ep.get() == endpoint;
  });

  if (k != pendingConnects_.end()) {
    pendingConnects_.erase(k);
  }
}

}  // namespace cortex
