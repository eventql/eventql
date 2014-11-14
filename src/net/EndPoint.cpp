// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/EndPoint.h>
#include <cassert>

namespace xzero {

EndPoint::EndPoint() XZERO_NOEXCEPT
    : connection_(nullptr) {
}

void EndPoint::setConnection(Connection* connection) {
  assert(connection_ == nullptr && "Cannot reassign connection.");
  connection_ = connection;
}

}  // namespace xzero
