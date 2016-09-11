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
#pragma once
#include "eventql/eventql.h"
#include "eventql/util/return_code.h"

namespace eventql {
namespace native_transport {

class NativeConnection {
public:

  static const size_t kMaxFrameSize = 1024 * 1024 * 256; // 256 MB
  static const size_t kMaxFrameSizeSoft = 1024 * 1024 * 32; // 32 MB

  virtual ~NativeConnection() = default;

  virtual ReturnCode recvFrame(
      uint16_t* opcode,
      uint16_t* flags,
      std::string* payload,
      uint64_t timeout_us) = 0;

  //virtual ReturnCode peekFrameAsync(
  //    uint16_t* opcode,
  //    uint16_t* flags = 0) = 0;

  virtual ReturnCode sendFrame(
      uint16_t opcode,
      uint16_t flags,
      const void* payload,
      size_t payload_len) = 0;

  virtual ReturnCode sendFrameAsync(
      uint16_t opcode,
      uint16_t flags,
      const void* data,
      size_t len) = 0;

  ReturnCode sendErrorFrame(const std::string& error);
  ReturnCode sendHeartbeatFrame();

  virtual bool isOutboxEmpty() const = 0;
  virtual ReturnCode flushOutbox(bool block, uint64_t timeout_us = 0) = 0;

  virtual void close() = 0;

  virtual std::string getRemoteHost() const = 0;

  virtual void setIOTimeout(uint64_t timeout_us) = 0;

};

} // namespace native_transport
} // namespace eventql

