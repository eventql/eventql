// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/Server.h>
#include <cortex-base/net/Connector.h>
#include <cortex-base/net/InetConnector.h>
#include <algorithm>

namespace cortex {

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

}  // namespace cortex
