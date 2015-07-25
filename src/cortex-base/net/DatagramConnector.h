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
#include <functional>
#include <string>

namespace cortex {

class Executor;
class DatagramEndPoint;

typedef std::function<void(RefPtr<DatagramEndPoint>)> DatagramHandler;

/**
 * Datagram Connector.
 *
 * @see DatagramConnector, DatagramEndPoint
 */
class CORTEX_API DatagramConnector {
 public:
  /**
   * Initializes the UDP connector.
   *
   * @param name Human readable name for the given connector (such as "ntp").
   * @param handler Callback handler to be invoked on every incoming message.
   * @param executor Executor service to be used for invoking the handler.
   */
  DatagramConnector(
      const std::string& name,
      DatagramHandler handler,
      Executor* executor);

  virtual ~DatagramConnector();

  DatagramHandler handler() const;
  void setHandler(DatagramHandler handler);

  /**
   * Starts handling incoming messages.
   */
  virtual void start() = 0;

  /**
   * Whether or not incoming messages are being handled.
   */
  virtual bool isStarted() const = 0;

  /**
   * Stops handling incoming messages.
   */
  virtual void stop() = 0;

 protected:
  std::string name_;
  DatagramHandler handler_;
  Executor* executor_;
};

} // namespace cortex
