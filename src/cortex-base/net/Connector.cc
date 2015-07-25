// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/Connector.h>
#include <cortex-base/net/ConnectionFactory.h>
#include <cortex-base/net/Server.h>
#include <algorithm>
#include <cassert>

namespace cortex {

Connector::Connector(const std::string& name, Executor* executor,
                     WallClock* clock)
  : name_(name),
    server_(nullptr),
    executor_(executor),
    clock_(clock),
    connectionFactories_(),
    defaultConnectionFactory_(),
    listeners_() {
}

Connector::~Connector() {
}

void Connector::setServer(Server* server) {
  assert(server_ == nullptr);
  server_ = server;
}

Server* Connector::server() const {
  return server_;
}

const std::string& Connector::name() const {
  return name_;
}

void Connector::setName(const std::string& name) {
  name_ = name;
}

std::shared_ptr<ConnectionFactory> Connector::addConnectionFactory(
    std::shared_ptr<ConnectionFactory> factory) {

  assert(factory.get() != nullptr);

  connectionFactories_[factory->protocolName()] = factory;

  if (connectionFactories_.size() == 1) {
    defaultConnectionFactory_ = factory;
  }

  return factory;
}

std::shared_ptr<ConnectionFactory> Connector::connectionFactory(const std::string& protocolName) const {
  auto i = connectionFactories_.find(protocolName);
  if (i != connectionFactories_.end()) {
    return i->second;
  }
  return nullptr;
}

std::list<std::shared_ptr<ConnectionFactory>> Connector::connectionFactories() const {
  std::list<std::shared_ptr<ConnectionFactory>> result;
  for (auto& entry: connectionFactories_) {
    result.push_back(entry.second);
  }
  return result;
}

void Connector::setDefaultConnectionFactory(std::shared_ptr<ConnectionFactory> factory) {
  auto i = connectionFactories_.find(factory->protocolName());
  if (i == connectionFactories_.end())
    throw std::runtime_error("Invalid argument.");

  if (i->second != factory)
    throw std::runtime_error("Invalid argument.");

  defaultConnectionFactory_ = factory;
}

std::shared_ptr<ConnectionFactory> Connector::defaultConnectionFactory() const {
  return defaultConnectionFactory_;
}

} // namespace cortex
