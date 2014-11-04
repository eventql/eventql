// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/Server.h>
#include <xzero/net/Connector.h>
#include <xzero/net/InetConnector.h>
#include <algorithm>

namespace xzero {

Server::Server()
    : connectors_(), date_() {
}

Server::Server(int port)
    : connectors_(), date_() {
  // TODO addConnector<InetConnector>(IPAddress("0.0.0.0"), port);
}

Server::Server(const IPAddress& address, int port)
    : connectors_(), date_() {
  // TODO addConnector<InetConnector>(address, port);
}

Server::~Server() {
  for (Connector* connector : connectors_) {
    delete connector;
  }
}

void Server::start() {
  for (Connector* connector : connectors_) {
    connector->start();
  }
}

void Server::stop() {
  for (Connector* connector : connectors_) {
    connector->stop();
  }
}

void Server::implAddConnector(Connector* connector) {
  connector->setServer(this);
  connectors_.push_back(connector);
}

void Server::removeConnector(Connector* connector) {
  auto i = std::find(connectors_.begin(), connectors_.end(), connector);
  if (i != connectors_.end()) {
    delete connector;
  }
}

std::list<Connector*> Server::getConnectors() const {
  return connectors_;
}

void Server::getDate(char* buf, size_t size) {
  if (size) {
    size_t n = std::min(size, date_.size()) - 1;
    memcpy(buf, date_.c_str(), n);
    buf[n] = '\0';
  }
}

}  // namespace xzero
