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
#include <cortex-base/IdleTimeout.h>
#include <cortex-base/thread/Future.h>
#include <cortex-base/net/EndPoint.h>
#include <cortex-base/net/IPAddress.h>
#include <atomic>

namespace cortex {

class InetConnector;

/**
 * TCP/IP endpoint, usually created by an InetConnector.
 */
class CORTEX_API InetEndPoint : public EndPoint {
 public:
  /**
   * Initializes a server-side InetEndPoint.
   */
  InetEndPoint(int socket, InetConnector* connector, Scheduler* scheduler);

  /**
   * Initializes a client-side InetEndPoint.
   */
  InetEndPoint(int socket, int addressFamily,
               TimeSpan readTimeout, TimeSpan writeTimeout,
               WallClock* clock, Scheduler* scheduler);

  ~InetEndPoint();

  static Future<std::unique_ptr<InetEndPoint>> connectAsync(
      const IPAddress& ipaddr, int port,
      TimeSpan timeout, WallClock* clock, Scheduler* scheduler);

  static void connectAsync(
      const IPAddress& ipaddr, int port,
      TimeSpan timeout, WallClock* clock, Scheduler* scheduler,
      std::function<void(std::unique_ptr<InetEndPoint>&&)> cb);

  static std::unique_ptr<InetEndPoint> connect(
      const IPAddress& ipaddr, int port,
      TimeSpan timeout, WallClock* clock, Scheduler* scheduler);

  int handle() const noexcept { return handle_; }

  /**
   * Returns the underlying address family, such as @c AF_INET or @c AF_INET6.
   */
  int addressFamily() const noexcept { return addressFamily_; }

  /**
   * Retrieves remote address + port.
   */
  std::pair<IPAddress, int> remoteAddress() const;

  /**
   * Retrieves local address + port.
   */
  std::pair<IPAddress, int> localAddress() const;

  // EndPoint overrides
  bool isOpen() const CORTEX_NOEXCEPT override;
  void close() override;
  bool isBlocking() const override;
  void setBlocking(bool enable) override;
  bool isCorking() const override;
  void setCorking(bool enable) override;
  std::string toString() const override;
  size_t fill(Buffer* result) override;
  size_t flush(const BufferRef& source) override;
  size_t flush(int fd, off_t offset, size_t size) override;
  void wantFill() override;
  void wantFlush() override;
  TimeSpan readTimeout() override;
  TimeSpan writeTimeout() override;
  void setReadTimeout(TimeSpan timeout) override;
  void setWriteTimeout(TimeSpan timeout) override;
  Option<IPAddress> remoteIP() const override;

 private:
  void onReadable() CORTEX_NOEXCEPT;
  void onWritable() CORTEX_NOEXCEPT;

  void fillable();
  void flushable();
  void onTimeout();

 private:
  InetConnector* connector_;
  Scheduler* scheduler_;
  TimeSpan readTimeout_;
  TimeSpan writeTimeout_;
  IdleTimeout idleTimeout_;
  Scheduler::HandleRef io_;
  int handle_;
  int addressFamily_;
  bool isCorking_;
};

} // namespace cortex
