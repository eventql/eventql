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
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/net/SslEndPoint.h>
#include <list>
#include <memory>
#include <openssl/ssl.h>

namespace cortex {

class SslContext;

/**
 * SSL Connector.
 *
 * @see InetConnector
 * @see SslEndPoint
 */
class CORTEX_API SslConnector : public InetConnector {
 public:
  /**
   * Initializes this connector.
   *
   * @param name Describing name for this connector.
   * @param executor Executor service to run handlers on
   * @param scheduler Scheduler service to use for scheduling tasks
   * @param clock Wall clock used for timeout management.
   * @param readTimeout timespan indicating how long a connection may be idle
   *                    for read operations.
   * @param writeTimeout timespan indicating how long a connection may be idle
   *                     for read operations.
   * @param tcpFinTimeout Timespan to leave client sockets in FIN_WAIT2 state.
   *                      A value of 0 means to leave it at system default.
   * @param eh exception handler for errors in hooks or during events.
   * @param ipaddress TCP/IP address to listen on
   * @param port TCP/IP port number to listen on
   * @param backlog TCP backlog for this listener.
   * @param reuseAddr Flag indicating @c SO_REUSEADDR.
   * @param reusePort Flag indicating @c SO_REUSEPORT.
   *
   * @throw std::runtime_error on any kind of runtime error.
   */
  SslConnector(const std::string& name, Executor* executor,
               Scheduler* scheduler, WallClock* clock,
               TimeSpan readTimeout, TimeSpan writeTimeout,
               TimeSpan tcpFinTimeout,
               std::function<void(const std::exception&)> eh,
               const IPAddress& ipaddress, int port, int backlog,
               bool reuseAddr, bool reusePort);
  ~SslConnector();

  /**
   * Adds a new SSL context (certificate & key) pair.
   *
   * You must at least add one SSL certificate. Adding more will implicitely
   * enable SNI support.
   */
  void addContext(const std::string& crtFilePath,
                  const std::string& keyFilePath);

  void start() override;
  bool isStarted() const CORTEX_NOEXCEPT override;
  void stop() override;
  std::list<RefPtr<EndPoint>> connectedEndPoints() override;

  RefPtr<EndPoint> createEndPoint(int cfd) override;
  void onEndPointCreated(const RefPtr<EndPoint>& endpoint) override;

  SslContext* selectContext(const char* servername) const;

 private:
  SslContext* defaultContext() const;

  static int selectContext(SSL* ssl, int* ad, SslConnector* connector);

  friend class SslEndPoint;
  friend class SslContext;

 private:
  std::list<std::unique_ptr<SslContext>> contexts_;
};

inline SslContext* SslConnector::defaultContext() const {
  return !contexts_.empty()
      ? contexts_.front().get()
      : nullptr;
}

} // namespace cortex


