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
#include "eventql/db/database.h"
#include "eventql/util/return_code.h"

namespace eventql {
namespace native_transport {
class NativeConnection;

class Server {
public:

  Server(Database* db);

  void startConnection(std::unique_ptr<NativeConnection> connection);

  ReturnCode performHandshake(NativeConnection* conn);

  ReturnCode performOperation(
      NativeConnection* conn,
      uint16_t opcode,
      const std::string& payload);

protected:
  Database* db_;
};

ReturnCode performOperation_QUERY(
    Database* database,
    NativeConnection* conn,
    const std::string& payload);

ReturnCode performOperation_QUERY_PARTIALAGGR(
    Database* database,
    NativeConnection* conn,
    const char* payload,
    size_t payload_size);


} // namespace native_transport
} // namespace eventql

