// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/Connection.h>
#include <cortex-base/net/EndPoint.h>
#include <algorithm>

namespace cortex {

Connection::Connection(EndPoint* endpoint,
                       Executor* executor)
    : endpoint_(endpoint),
      executor_(executor),
      listeners_() {

  if (endpoint_) {
    endpoint_->setConnection(this);
  }
}

Connection::~Connection() {
}

void Connection::onOpen() {
  for (ConnectionListener* listener : listeners_) {
    listener->onOpened(this);
  }
}

void Connection::onClose() {
  for (ConnectionListener* listener : listeners_) {
    listener->onClosed(this);
  }
}

void Connection::addListener(ConnectionListener* listener) {
  listeners_.push_back(listener);
}

void Connection::close() {
  if (endpoint_) {
    endpoint_->close();
  }
}

void Connection::setInputBufferSize(size_t size) {
  // default no-op
}

void Connection::wantFill() {
  // register read-event
  endpoint()->wantFill();
}

void Connection::wantFlush() {
  // register write-event
  endpoint()->wantFlush();
}

void Connection::onInterestFailure(const std::exception& error) {
  close();
}

bool Connection::onReadTimeout() {
  // inform caller to close the endpoint
  return true;
}

// {{{ ConnectionListener impl
ConnectionListener::~ConnectionListener() {
}

void ConnectionListener::onOpened(Connection* connection) {
}

void ConnectionListener::onClosed(Connection* connection) {
}
// }}}

}  // namespace cortex
