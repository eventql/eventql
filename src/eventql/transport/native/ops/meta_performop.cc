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
#include "eventql/transport/native/connection.h"
#include "eventql/transport/native/frames/meta_performop.h"
#include "eventql/util/logging.h"
#include "eventql/util/wallclock.h"
#include "eventql/server/session.h"
#include "eventql/db/metadata_service.h"

namespace eventql {
namespace native_transport {

ReturnCode performOperation_META_PERFORMOP(
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

  /* parse request */
  MetaPerformopFrame m_frame;
  {
    MemoryInputStream is(payload, payload_size);
    auto rc = m_frame.parseFrom(&is);
    if (!rc.isSuccess()) {
      return conn->sendErrorFrame("invalid frame");
    }
  }

  MetadataOperation op;
  {
    auto is = StringInputStream::fromString(m_frame.getOperation());
    auto rc = op.decode(is.get());
    if (!rc.isSuccess()) {
      return conn->sendErrorFrame("invalid frame");
    }
  }

  /* execute operation */
  auto t0 = MonotonicClock::now();
  MetadataOperationResult result;
  auto rc = dbctx->metadata_service->performMetadataOperation(
      m_frame.getDatabase(),
      m_frame.getTable(),
      op,
      &result);
  auto t1 = MonotonicClock::now();

  logInfo(
      "evqld",
      "Performing metadata operation $0 on $1/$2 ($3->$4), took $5ms",
      MetadataOperationType_Name(op.getOperationType()),
      m_frame.getDatabase(),
      m_frame.getTable(),
      op.getInputTransactionID(),
      op.getOutputTransactionID(),
      double(t1-t0)/1000.f);

  /* send response */
  if (rc.isSuccess()) {
    auto payload = *msg::encode(result);

    return conn->sendFrame(
        EVQL_OP_META_PERFORMOP_RESULT,
        EVQL_ENDOFREQUEST,
        payload.data(),
        payload.size());
  } else {
    return conn->sendErrorFrame(rc.message());
  }

  return ReturnCode::success();
}

} // namespace native_transport
} // namespace eventql

