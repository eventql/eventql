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
#include <cortex-base/Buffer.h>

namespace cortex {

class IPAddress;

/**
 * Simple UDP client.
 */
class UdpClient {
 public:
  UdpClient(const IPAddress& ip, int port);
  ~UdpClient();

  /**
   * Receives the underlying system handle for given UDP communication.
   *
   * Use this for Event registration in Scheduler API.
   */
  int handle() const noexcept;

  size_t send(const BufferRef& message);
  size_t receive(Buffer* message);

 private:
  int socket_;
  int addressFamily_;
  void* sockAddr_;
  int sockAddrLen_;
};

inline int UdpClient::handle() const noexcept {
  return socket_;
}

} // namespace cortex
