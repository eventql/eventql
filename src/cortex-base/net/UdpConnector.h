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
#include <cortex-base/executor/Scheduler.h>
#include <cortex-base/net/DatagramConnector.h>
#include <cortex-base/net/IPAddress.h>
#include <functional>

namespace cortex {

class Scheduler;

/**
 * Datagram Connector for UDP protocol.
 *
 * @see DatagramConnector, DatagramEndPoint
 */
class CORTEX_API UdpConnector : public DatagramConnector {
 public:
  /**
   * Initializes the UDP connector.
   *
   * @param name Human readable name for the given connector (such as "ntp").
   * @param handler Callback handler to be invoked on every incoming message.
   * @param executor Executor service to be used for invoking the handler.
   * @param scheduler Scheduler service to be used for I/O notifications.
   * @param ipaddress IP address to bind the connector to.
   * @param port UDP port number to bind the connector to.
   * @param reuseAddr Whether or not to enable @c SO_REUSEADDR.
   * @param reusePort Whether or not to enable @c SO_REUSEPORT.
   */
  UdpConnector(
      const std::string& name,
      DatagramHandler handler,
      Executor* executor,
      Scheduler* scheduler,
      const IPAddress& ipaddress, int port,
      bool reuseAddr, bool reusePort);

  ~UdpConnector();

  int handle() const noexcept { return socket_; }

  void start() override;
  bool isStarted() const override;
  void stop() override;

 private:
  void open(
      const IPAddress& bind, int port,
      bool reuseAddr, bool reusePort);

  void notifyOnEvent();
  void onMessage();

 private:
  Scheduler* scheduler_;
  Scheduler::HandleRef schedulerHandle_;
  int socket_;
  int addressFamily_;
};

} // namespace cortex
