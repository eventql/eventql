// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/Connection.h>
#include <xzero/net/EndPoint.h>
#include <algorithm>

namespace xzero {

Connection::Connection(std::shared_ptr<EndPoint> endpoint,
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

void Connection::wantFlush(bool enable) {
  // register write-event
  endpoint()->wantFlush(enable);
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

}  // namespace xzero
