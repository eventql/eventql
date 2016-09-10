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
#include "eventql/transport/native/connection.h"

namespace eventql {
namespace native_transport {

class TCPConnection : public NativeConnection {
public:

  TCPConnection(
      int fd,
      uint64_t io_timeout,
      const std::string& prelude_bytes = "");

  ~TCPConnection();

  ReturnCode recvFrame(
      uint16_t* opcode,
      uint16_t* flags,
      std::string* payload,
      uint64_t timeout_us) override;

  ReturnCode sendFrame(
      uint16_t opcode,
      uint16_t flags,
      const void* payload,
      size_t payload_len) override;

  ReturnCode sendFrameAsync(
      uint16_t opcode,
      uint16_t flags,
      const void* payload,
      size_t payload_len) override;

  bool isOutboxEmpty() const override;
  ReturnCode flushOutbox(bool block, uint64_t timeout_us = 0) override;

  void close() override;

  std::string getRemoteHost() const override;

  void setIOTimeout(uint64_t timeout_us) override;

protected:

  void writeFrameHeaderAsync(
      uint16_t opcode,
      size_t len,
      uint16_t flags = 0);

  void writeAsync(const void* data, size_t len);

  ReturnCode read(char* data, size_t len, uint64_t timeout_us);

  int fd_;
  std::string read_buf_;
  std::string write_buf_;
  uint64_t io_timeout_;
  std::string remote_host_;
};

} // namespace native_transport
} // namespace eventql

