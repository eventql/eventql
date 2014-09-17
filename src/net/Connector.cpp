#include <xzero/net/Connector.h>
#include <xzero/net/ConnectionFactory.h>
#include <xzero/net/Server.h>
#include <cassert>

namespace xzero {

Connector::Connector(const std::string& name, Executor* executor)
  : name_(name),
    server_(nullptr),
    executor_(executor),
    connectionFactories_(),
    defaultConnectionFactory_(),
    listeners_() {
}

Connector::~Connector() {
  for (auto& factory: connectionFactories_) {
    /* delete factory.second; */
  }
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
  defaultConnectionFactory_ = factory;
}

std::shared_ptr<ConnectionFactory> Connector::defaultConnectionFactory() const {
  return defaultConnectionFactory_;
}

} // namespace xzero
