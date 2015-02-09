// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

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

  executor()->execute(std::bind(&LocalConnector::acceptOne, this));

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

}  // namespace xzero
