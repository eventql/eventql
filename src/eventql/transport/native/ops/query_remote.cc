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
#include "eventql/transport/native/server.h"
#include "eventql/transport/native/connection_tcp.h"
#include "eventql/transport/native/frames/query_remote.h"
#include "eventql/transport/native/frames/query_remote_result.h"
#include "eventql/util/logging.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/util/binarymessagereader.h"
#include "eventql/db/database.h"
#include "eventql/server/session.h"
#include "eventql/server/sql_service.h"
#include "eventql/sql/runtime/runtime.h"
#include "eventql/auth/client_auth.h"

namespace eventql {
namespace native_transport {

ReturnCode performOperation_QUERY_REMOTE(
    Database* database,
    NativeConnection* conn,
    const char* payload,
    size_t payload_size) {
  auto session = database->getSession();
  auto dbctx = session->getDatabaseContext();

  /* check internal */
  if (!session->isInternal()) {
    return conn->sendErrorFrame("internal method called");
  }

  /* read query frame */
  QueryRemoteFrame q_frame;
  {
    auto rc = q_frame.parseFrom(payload, payload_size);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  /* set heartbeat callback */
  session->setHeartbeatCallback([conn] () -> ReturnCode {
    return conn->sendHeartbeatFrame();
  });

  /* switch database */
  {
    auto rc = dbctx->client_auth->changeNamespace(
        session,
        q_frame.getDatabase());
    if (!rc.isSuccess()) {
      return conn->sendErrorFrame(rc.message());
    }
  }

  /* execute queries */
  try {
    auto txn = dbctx->sql_service->startTransaction(session);

    csql::QueryTreeCoder coder(txn.get());
    auto req_body_is = StringInputStream::fromString(
        q_frame.getEncodedQTree());
    auto qtree = coder.decode(req_body_is.get());
    auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), { qtree });

    /* execute query */
    auto result_cursor = qplan->execute(0);
    auto result_ncols = result_cursor->getNumColumns();

    /* send response frames */
    QueryRemoteResultFrame r_frame;
    r_frame.setColumnCount(result_ncols);
    auto r_frame_data_os = r_frame.getRowDataOutputStream();
    Vector<csql::SValue> row(result_ncols);
    while (result_cursor->next(row.data(), row.size())) {
      r_frame.setRowCount(r_frame.getRowCount() + 1);
      for (size_t i = 0; i < result_ncols; ++i) {
        row[i].encode(r_frame_data_os.get());
      }

      if (r_frame.getRowBytes() > NativeConnection::kMaxFrameSizeSoft) {
        {
          std::string r_payload;
          auto payload_os = StringOutputStream::fromString(&r_payload);
          r_frame.writeTo(payload_os.get());

          auto rc = conn->sendFrame(
              EVQL_OP_QUERY_REMOTE_RESULT,
              0,
              r_payload.data(),
              r_payload.size());

          if (!rc.isSuccess()) {
            return rc;
          }
        }

        r_frame.clear();
        r_frame.setColumnCount(result_ncols);

        /* wait for discard or continue */
        uint16_t n_opcode;
        uint16_t n_flags;
        std::string n_payload;
        {
          auto rc = conn->recvFrame(
              &n_opcode,
              &n_flags,
              &n_payload,
              session->getIdleTimeout());

          if (!rc.isSuccess()) {
            return rc;
          }
        }

        bool cont = true;
        switch (n_opcode) {
          case EVQL_OP_QUERY_CONTINUE:
            break;
          case EVQL_OP_QUERY_DISCARD:
            cont = false;
            break;
          default:
            conn->close();
            return ReturnCode::error("ERUNTIME", "unexpected opcode");
        }

        if (!cont) {
          break;
        }
      }
    }

    std::string r_payload;
    auto payload_os = StringOutputStream::fromString(&r_payload);
    r_frame.writeTo(payload_os.get());

    auto rc = conn->sendFrame(
        EVQL_OP_QUERY_REMOTE_RESULT,
        EVQL_ENDOFREQUEST,
        r_payload.data(),
        r_payload.size());

    if (!rc.isSuccess()) {
      return rc;
    }
  } catch (const StandardException& e) {
    return conn->sendErrorFrame(e.what());
  }

  return ReturnCode::success();
}

} // namespace native_transport
} // namespace eventql

