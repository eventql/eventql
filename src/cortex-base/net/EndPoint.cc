// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/EndPoint.h>
#include <cortex-base/net/Connection.h>
#include <cassert>

namespace cortex {

EndPoint::EndPoint() CORTEX_NOEXCEPT
    : connection_(nullptr) {
}

EndPoint::~EndPoint() {
  delete connection_;
}

void EndPoint::setConnection(Connection* connection) {
  assert(connection_ == nullptr && "Cannot reassign connection.");
  connection_ = connection;
}

Option<IPAddress> EndPoint::remoteIP() const {
  return None();
}

}  // namespace cortex
