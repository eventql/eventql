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
#include "eventql/transport/native/connection.h"
#include "eventql/util/util/binarymessagewriter.h"

namespace eventql {
namespace native_transport {

ReturnCode NativeConnection::sendErrorFrame(const std::string& error) {
  util::BinaryMessageWriter e_frame;
  e_frame.appendLenencString(error);
  char zero = 0;
  e_frame.append(&zero, 1);

  return sendFrame(
      EVQL_OP_ERROR,
      EVQL_ENDOFREQUEST,
      e_frame.data(),
      e_frame.size());
}

ReturnCode NativeConnection::sendHeartbeatFrame() {
  if (isOutboxEmpty()) {
    return sendFrameAsync(EVQL_OP_HEARTBEAT, 0, nullptr, 0);
  } else {
    return flushOutbox(false, 0);
  }
}

} // namespace native_transport
} // namespace eventql

