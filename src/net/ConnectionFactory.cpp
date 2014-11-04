// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/ConnectionFactory.h>
#include <xzero/net/Connector.h>
#include <xzero/net/Connection.h>

namespace xzero {

ConnectionFactory::ConnectionFactory(const std::string& protocolName) :
  protocolName_(protocolName),
  inputBufferSize_(8192) {
}

ConnectionFactory::~ConnectionFactory() {
}

const std::string& ConnectionFactory::protocolName() const {
  return protocolName_;
}

size_t ConnectionFactory::inputBufferSize() const {
  return inputBufferSize_;
}

void ConnectionFactory::setInputBufferSize(size_t value) {
  inputBufferSize_ = value;
}

std::string ConnectionFactory::toString() const {
  char buf[256];
  snprintf(buf, sizeof(buf), "ConnectionFactory@%p(%s)", this,
           protocolName().c_str());
  return buf;
}

Connection* ConnectionFactory::configure(Connection* connection,
                                         Connector* connector) {

  connection->setInputBufferSize(inputBufferSize_);

  if (connector != nullptr) {
    for (ConnectionListener* listener: connector->listeners()) {
      connection->addListener(listener);
    }
  }

  return connection;
}

}  // namespace xzero
