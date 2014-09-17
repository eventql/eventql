#include <xzero/net/EndPoint.h>
#include <cassert>

namespace xzero {

EndPoint::EndPoint() noexcept
    : connection_(nullptr) {
}

void EndPoint::setConnection(Connection* connection) {
  assert(connection_ == nullptr && "Cannot reassign connection.");
  connection_ = connection;
}

}  // namespace xzero
