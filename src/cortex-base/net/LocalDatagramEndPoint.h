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
#include <cortex-base/net/DatagramEndPoint.h>
#include <vector>

namespace cortex {

class LocalDatagramConnector;

/**
 * In-memory based datagram endpoint.
 *
 * @see LocalDatagramConnector.
 */
class CORTEX_API LocalDatagramEndPoint : public DatagramEndPoint {
 public:
  LocalDatagramEndPoint(LocalDatagramConnector* connector, Buffer&& msg);
  ~LocalDatagramEndPoint();

  size_t send(const BufferRef& response) override;

  const std::vector<Buffer>& responses() const noexcept { return responses_; }

 private:
  std::vector<Buffer> responses_;
};

} // namespace cortex
