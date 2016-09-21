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
#include "eventql/transport/native/server.h"
#include "eventql/transport/native/connection.h"
#include "eventql/transport/native/frames/repl_insert.h"
#include "eventql/auth/client_auth.h"
#include "eventql/util/logging.h"
#include "eventql/server/session.h"
#include <eventql/db/shredded_record.h>
#include <eventql/db/table_service.h>

namespace eventql {
namespace native_transport {

ReturnCode performOperation_REPL_INSERT(
    Database* database,
    NativeConnection* conn,
    const char* payload,
    size_t payload_size) {
  auto session = database->getSession();
  auto dbctx = session->getDatabaseContext();

  session->setHeartbeatCallback([conn] () -> ReturnCode {
    return conn->sendHeartbeatFrame();
  });

  /* check internal */
  if (!session->isInternal()) {
    return conn->sendErrorFrame("internal method called");
  }

  ReplInsertFrame i_frame;
  {
    MemoryInputStream payload_is(payload, payload_size);
    auto rc = i_frame.parseFrom(&payload_is);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  /* switch database */
  {
    auto rc = dbctx->client_auth->changeNamespace(
        session,
        i_frame.getDatabase());

    if (!rc.isSuccess()) {
      return conn->sendErrorFrame(rc.message());
    }
  }

  /* load shredded record list */
  ShreddedRecordList records;
  auto body_is = StringInputStream::fromString(i_frame.getBody());
  records.decode(body_is.get());

  /* perform insert */
  auto rc = dbctx->table_service->insertReplicatedRecords(
      session->getEffectiveNamespace(),
      i_frame.getTable(),
      SHA1Hash::fromHexString(i_frame.getPartitionID()),
      records);

  if (rc.isSuccess()) {
    conn->sendFrame(EVQL_OP_ACK, EVQL_ENDOFREQUEST, nullptr, 0);
  } else {
    conn->sendErrorFrame(rc.getMessage());
  }

  return ReturnCode::success();
}

} // namespace native_transport
} // namespace eventql

