// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/sysconfig.h>
#include <memory>
#include <list>

namespace cortex {

class Executor;
class Connector;
class EndPoint;
class ConnectionListener;

/**
 * A Connection is responsible for processing an EndPoint.
 *
 * A Connection derivate can implement various stream oriented protocols.
 * This doesn't necessarily has to be HTTP, but can also be SMTP or anything
 * else.
 */
class CORTEX_API Connection {
 public:
  Connection(EndPoint* endpoint, Executor* executor);
  virtual ~Connection();

  /**
   * Callback, invoked when connection was opened.
   */
  virtual void onOpen();

  /**
   * Callback, invoked when connection is closed.
   */
  virtual void onClose();

  /**
   * Retrieves the corresponding endpoint for this connection.
   */
  EndPoint* endpoint() const CORTEX_NOEXCEPT;

  /**
   * Retrieves the Executor that may be used for handling this connection.
   */
  Executor* executor() const CORTEX_NOEXCEPT;

  /**
   * Registers given @p listener to this connection.
   *
   * @param listener the listener to register.
   *
   * @see void Connector::addListener(ConnectionListener* listener)
   */
  void addListener(ConnectionListener* listener);

  /**
   * Closes the underlying endpoint.
   */
  virtual void close();

  /**
   * Configures the input buffer size for this Connection.
   *
   * @param size number of bytes the input buffer for this connection may use.
   */
  virtual void setInputBufferSize(size_t size);

  /**
   * Ensures onFillable() is invoked when data is available for
   * read.
   *
   * In any case of an error, onInterestFailure(const std::exception&) is invoked.
   */
  void wantFill();

  /**
   * Ensures onFlushable() is invoked when underlying endpoint is ready
   * to write.
   *
   * In any case of an error, onInterestFailure(const std::exception&) is invoked.
   */
  void wantFlush();

  /**
   * Event callback being invoked when data is available for read.
   */
  virtual void onFillable() = 0;

  /**
   * Event callback being invoked when underlying endpoint ready for write.
   */
  virtual void onFlushable() = 0;

  /**
   * Event callback being invoked on any errors while waiting for data.
   *
   * For example read timeout (or possibly connection timeout?).
   *
   * The default implementation simply invokes abort().
   */
  virtual void onInterestFailure(const std::exception& error);

  /**
   * Callback being invoked when a read-timeout has been reached.
   *
   * @retval true close the endpoint.
   * @retval false ignore the timeout, do not close.
   *
   * By default the implementation of Connection::onReadTimeout() returns just
   * true.
   */
  virtual bool onReadTimeout();

 private:
  EndPoint* endpoint_;
  Executor* executor_;
  std::list<ConnectionListener*> listeners_;
};

inline EndPoint* Connection::endpoint() const CORTEX_NOEXCEPT {
  return endpoint_;
}

inline Executor* Connection::executor() const CORTEX_NOEXCEPT {
  return executor_;
}

/**
 * Connection event listener, such as on-open and on-close events.
 *
 * @see void Connector::addListener(ConnectionListener* listener)
 */
class CORTEX_API ConnectionListener {
 public:
  virtual ~ConnectionListener();

  /** Invoked via Conection::onOpen(). */
  virtual void onOpened(Connection* connection);

  /** Invoked via Conection::onClose(). */
  virtual void onClosed(Connection* connection);
};

} // namespace cortex
