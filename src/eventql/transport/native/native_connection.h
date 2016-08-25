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

class NativeConnection{
public:

  static const size_t kMaxFrameSize = 1024 * 1024 * 256; // 256 MB

  NativeConnection(
      int fd,
      const std::string& prelude_bytes = "");

  ~NativeConnection();

  ReturnCode recvFrame(
      uint16_t* opcode,
      std::string* payload,
      uint16_t* flags = nullptr);

  void close();

protected:

  ReturnCode read(
      char* data,
      size_t len,
      uint64_t timeout_us);

  ReturnCode write(
      char* data,
      size_t len,
      uint64_t timeout_us);

  int fd_;
  uint64_t timeout_;
  std::string read_buf_;
};

} // namespace eventql
