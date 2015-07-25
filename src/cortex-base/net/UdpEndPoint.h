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
#include <cortex-base/net/DatagramEndPoint.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/Buffer.h>

namespace cortex {

class UdpConnector;
class IPAddress;

class CORTEX_API UdpEndPoint : public DatagramEndPoint {
 public:
  UdpEndPoint(
      UdpConnector* connector, Buffer&& msg,
      struct sockaddr* remoteSock, int remoteSockLen);
  ~UdpEndPoint();

  size_t send(const BufferRef& response) override;

 private:
  Buffer remoteSock_;
};

} // namespace cortex

