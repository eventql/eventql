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
#include <cortex-base/net/Connector.h>
#include <cortex-base/net/ByteArrayEndPoint.h>
#include <list>
#include <deque>

namespace cortex {

class LocalConnector;

/**
 * The EndPoint for the LocalConnector API.
 */
class LocalEndPoint : public ByteArrayEndPoint {
 public:
  explicit LocalEndPoint(LocalConnector* connector);
  ~LocalEndPoint();

  void close() override;

 private:
  LocalConnector* connector_;
};

/**
 * Local Server Connector for injecting crafted HTTP client connections.
 *
 * Use LocalConnector in order to inject crafted HTTP client connections, such
 * as a custom HTTP request in a byte buffer.
 *
 * This API is ideal for unit-testing your server logic.
 *
 * @note The LocalConnector is always performing single threaded blocking I/O.
 */
class CORTEX_API LocalConnector : public Connector {
 public:
  explicit LocalConnector(Executor* executor = nullptr);
  ~LocalConnector();

  void start() override;
  bool isStarted() const CORTEX_NOEXCEPT override;
  void stop() override;
  std::list<RefPtr<EndPoint>> connectedEndPoints() override;

  RefPtr<LocalEndPoint> createClient(const std::string& rawRequestMessage);

 private:
  bool acceptOne();

  /**
   * Invoked internally by LocalEndPoint to actually destroy this object.
   */
  void release(Connection* localConnection);

  friend class LocalEndPoint;
  friend class ByteArrayEndPoint;
  void onEndPointClosed(LocalEndPoint* endpoint);

 private:
  bool isStarted_;
  std::deque<RefPtr<LocalEndPoint>> pendingConnects_;
  std::list<RefPtr<LocalEndPoint>> connectedEndPoints_;
};

inline bool LocalConnector::isStarted() const CORTEX_NOEXCEPT {
  return isStarted_;
}

} // namespace cortex
