// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/LocalDatagramEndPoint.h>
#include <cortex-base/net/LocalDatagramConnector.h>

namespace cortex {

LocalDatagramEndPoint::LocalDatagramEndPoint(
    LocalDatagramConnector* connector, Buffer&& msg)
    : DatagramEndPoint(connector, std::move(msg)) {
}

LocalDatagramEndPoint::~LocalDatagramEndPoint() {
}

size_t LocalDatagramEndPoint::send(const BufferRef& response) {
  responses_.emplace_back(response);
  return response.size();
}

} // namespace cortex
