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
#include "eventql/transport/native/native_transport.h"
#include "eventql/transport/native/native_connection.h"
#include "eventql/transport/native/frames/query_partial_aggr.h"
#include "eventql/util/logging.h"
#include "eventql/util/util/binarymessagereader.h"
#include "eventql/server/session.h"
#include "eventql/server/sql_service.h"
#include "eventql/sql/runtime/runtime.h"
#include "eventql/auth/client_auth.h"

namespace eventql {
namespace native_transport {

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

  try {
    auto txn = dbctx->sql_service->startTransaction(session);

    csql::QueryTreeCoder coder(txn.get());

    auto req_body_is = StringInputStream::fromString(frame.getEncodedQTree());
    auto qtree = coder.decode(req_body_is.get());
    auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), { qtree });
    auto execute = qplan->execute(0);
    csql::SValue row[2];
    while (execute->next(row, 2)) {
      iputs("got row", 1);
    }

  } catch (const std::exception& e) {
    conn->sendErrorFrame(e.what());
  }

  return ReturnCode::success();
}

} // namespace native_transport
} // namespace eventql

