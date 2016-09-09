/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/transport/native/frames/rpc_result.h"
#include "eventql/util/util/binarymessagewriter.h"

namespace eventql {
namespace native_transport {

//RPCResultFrame::RPCResultFrame() : flags_(0) {}
//
//void RPCResultFrame::setBody(std::string body) {
//  body_ = body;
//}
//
//void RPCResultFrame::setBody(const char* data, size_t len) {
//  body_ = std::string(data, len);
//}
//
//void RPCResultFrame::setIsComplete(bool is_complete) {
//  if (is_complete) {
//    flags_ |= EVQL_RPC_RESULT_COMPLETE;
//  } else {
//    flags_ &= ~EVQL_RPC_RESULT_COMPLETE;
//  }
//}
//
//const std::string& RPCResultFrame::getBody() const {
//  return body_;
//}
//
//std::string& RPCResultFrame::getBody() {
//  return body_;
//}
//
//bool RPCResultFrame::getIsComplete() {
//  return flags_ & EVQL_RPC_RESULT_COMPLETE;
//}
//
//ReturnCode RPCResultFrame::writeTo(NativeConnection* conn) {
//  util::BinaryMessageWriter header_;
//  header_.appendVarUInt(flags_); // flags
//  header_.appendVarUInt(body_.size());
//
//  conn->writeFrameHeaderAsync(
//      EVQL_OP_RPC_RESULT,
//      header_.size() + body_.size());
//
//  conn->writeAsync(header_.data(), header_.size());
//  conn->writeAsync(body_.data(), body_.size());
//  return conn->flushBuffer(true);
//}
//

} // namespace native_transport
} // namespace eventql


