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
#include "eventql/transport/native/connection_tcp.h"
#include "eventql/transport/native/frames/query_partialaggr.h"
#include "eventql/transport/native/frames/query_partialaggr_result.h"
#include "eventql/util/logging.h"
#include "eventql/util/util/binarymessagereader.h"
#include "eventql/server/session.h"
#include "eventql/server/sql_service.h"
#include "eventql/sql/runtime/runtime.h"
#include "eventql/auth/client_auth.h"

namespace eventql {
namespace native_transport {

static const size_t kPartialAggrResponseSoftMaxSize = 1024 * 1024 * 8; // 8MB

ReturnCode performOperation_QUERY_PARTIALAGGR(
    Database* database,
    NativeConnection* conn,
    const char* payload,
    size_t payload_size) {
  QueryPartialAggrFrame frame;
  {
    auto rc = frame.parseFrom(payload, payload_size);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  auto session = database->getSession();
  auto dbctx = session->getDatabaseContext();
  /* switch database */
  {
    auto rc = dbctx->client_auth->changeNamespace(session, frame.getDatabase());
    if (!rc.isSuccess()) {
      return conn->sendErrorFrame(rc.message());
    }
  }

  session->setHeartbeatCallback([conn] () -> ReturnCode {
    return conn->sendHeartbeatFrame();
  });

  try {
    auto txn = dbctx->sql_service->startTransaction(session);

    csql::QueryTreeCoder coder(txn.get());

    auto req_body_is = StringInputStream::fromString(frame.getEncodedQTree());
    auto qtree = coder.decode(req_body_is.get());
    auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), { qtree });
    auto execute = qplan->execute(0);

    for (bool eof = false; !eof; ) {
      QueryPartialAggrResultFrame result_frame;
      auto os = StringOutputStream::fromString(&result_frame.getBody());
      csql::SValue row[2];
      size_t num_rows = 0;

      while ((eof = !execute->next(row, 2)) == false) {
        ++num_rows;
        os->appendLenencString(row[0].getString());
        os->appendString(row[1].getString());

        auto body_len = result_frame.getBody().size();
        if (body_len > kPartialAggrResponseSoftMaxSize) {
          break;
        }
      }

      result_frame.setNumRows(num_rows);

      std::string payload;
      auto payload_os = StringOutputStream::fromString(&payload);
      result_frame.writeTo(payload_os.get());

      auto rc = conn->sendFrame(
          EVQL_OP_QUERY_PARTIALAGGR_RESULT,
          eof ? EVQL_ENDOFREQUEST : 0,
          payload.data(),
          payload.size());

      if (!rc.isSuccess()) {
        return rc;
      }
    }
  } catch (const std::exception& e) {
    conn->sendErrorFrame(e.what());
  }

  return ReturnCode::success();
}

} // namespace native_transport
} // namespace eventql

