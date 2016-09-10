/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/transport/native/frames/hello.h"

namespace eventql {
namespace native_transport {

HelloFrame::HelloFrame() : flags_(0), idle_timeout_(0) {}

void HelloFrame::setIsInternal(bool is_internal) {
  if (is_internal) {
    flags_ |= EVQL_HELLO_INTERNAL;
  } else {
    flags_ &= ~EVQL_HELLO_INTERNAL;
  }
}

bool HelloFrame::isInternal() const {
  return flags_ & EVQL_HELLO_INTERNAL;
}

void HelloFrame::setIdleTimeout(uint64_t idle_timeout) {
  idle_timeout_ = idle_timeout;
}

uint64_t HelloFrame::getIdleTimeout() const {
  return idle_timeout_;
}

ReturnCode HelloFrame::readFrom(InputStream* is) {
  auto v = is->readVarUInt();
  if (v != 1) {
    return ReturnCode::error("ERUNTIME", "invalid protocol version");
  }

  auto evql_version = is->readLenencString();
  flags_ = is->readVarUInt();
  idle_timeout_ = is->readVarUInt();
  return ReturnCode::success();
}

void HelloFrame::writeTo(OutputStream* os) {
  os->appendVarUInt(1);
  os->appendLenencString(EVQL_VERSION);
  os->appendVarUInt(flags_);
  os->appendVarUInt(idle_timeout_);
}

} // namespace native_transport
} // namespace eventql
