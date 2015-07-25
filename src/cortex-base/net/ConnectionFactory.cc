// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/ConnectionFactory.h>
#include <cortex-base/net/Connector.h>
#include <cortex-base/net/Connection.h>

namespace cortex {

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

}  // namespace cortex
