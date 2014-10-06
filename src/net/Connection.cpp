#include <xzero/net/Connection.h>
#include <xzero/net/EndPoint.h>
#include <algorithm>

namespace xzero {

Connection::Connection(std::shared_ptr<EndPoint> endpoint)
    : endpoint_(endpoint), listeners_() {

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

size_t Connection::getMessagesIn() const {
  return 0;
}

size_t Connection::getMessagesOut() const {
  return 0;
}

size_t Connection::getBytesIn() const {
  return 0;
}

size_t Connection::getBytesOut() const {
  return 0;
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
  abort();
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
