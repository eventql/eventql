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
#include <cortex-base/RefPtr.h>
#include <string>

namespace cortex {

class Connection;
class Connector;
class EndPoint;

/**
 * Connection factory.
 *
 * A connection factory is instanciating connection objects
 * that are responsible processing endpoints, created by a connector.
 *
 * Possible protocol-names:
 * <dl>
 *  <dt>http</dt><dd>cleartext @c HTTP/1</dd>
 *  <dt>spdy/3</dt><dd>cleartext @c SPDY/3</dd>
 *  <dt>SSL-FOO</dt><dd>@c FOO chained over SSL,
 *      such as SSL-http for "HTTP over SSL"</dd>
 *  <dt>SSL-npn</dt><dd>protocol chosen via @c NPN, over @c SSL</dd>
 * </dl>
 *
 * @see Connector
 * @see Connection
 */
class CORTEX_API ConnectionFactory {
 private:
  ConnectionFactory(ConnectionFactory&) = delete;
  ConnectionFactory& operator=(ConnectionFactory&) = delete;

 public:
  explicit ConnectionFactory(const std::string& protocolName);
  virtual ~ConnectionFactory();

  /**
   * Retrieves the unique protocol name this factory is creating connections
   * for.
   */
  const std::string& protocolName() const;

  /**
   * Creates a new Connection instance with the given @p connector
   * and @p endpoint.
   *
   * @param connector the Connector that accepted the incoming connection.
   * @param endpoint the endpoint that corresponds to this connection.
   */
  virtual Connection* create(Connector* connector, EndPoint* endpoint) = 0;

  virtual std::string toString() const;

  /**
   * Retrieves the initial Connection input buffer size.
   */
  size_t inputBufferSize() const;

  /**
   * Sets the initial input buffer size for newly created Connection objects.
   */
  void setInputBufferSize(size_t value);

 protected:
  /**
   * Configures the newly created @p connection.
   *
   * Sets up @c inputBufferSize and ConnectionListener by default
   */
  virtual Connection* configure(Connection* connection, Connector* connector);

 private:
  std::string protocolName_;
  size_t inputBufferSize_;
};

}  // namespace cortex
