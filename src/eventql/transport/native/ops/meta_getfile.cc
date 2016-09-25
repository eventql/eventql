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
#include "eventql/transport/native/frames/meta_getfile.h"
#include "eventql/util/logging.h"
#include "eventql/server/session.h"
#include "eventql/db/metadata_service.h"

namespace eventql {
namespace native_transport {

ReturnCode performOperation_META_GETFILE(
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
  MetaGetfileFrame m_frame;
  {
    MemoryInputStream is(payload, payload_size);
    auto rc = m_frame.parseFrom(&is);
    if (!rc.isSuccess()) {
      return conn->sendErrorFrame("invalid frame");
    }
  }

  /* execute operation */
  RefPtr<MetadataFile> file;
  if (m_frame.getLatestTransactionFlag()) {
    auto rc = dbctx->metadata_service->getMetadataFile(
        m_frame.getDatabase(),
        m_frame.getTable(),
        &file);

    if (!rc.isSuccess()) {
      return conn->sendErrorFrame(rc.message());
    }
  } else {
    auto rc = dbctx->metadata_service->getMetadataFile(
        m_frame.getDatabase(),
        m_frame.getTable(),
        m_frame.getTransactionID(),
        &file);

    if (!rc.isSuccess()) {
      return conn->sendErrorFrame(rc.message());
    }
  }

  /* send response */
  std::string res_payload;
  auto os = StringOutputStream::fromString(&res_payload);
  auto rc = file->encode(os.get());
  if (rc.isSuccess()) {
    return conn->sendFrame(
        EVQL_OP_META_GETFILE_RESULT,
        EVQL_ENDOFREQUEST,
        res_payload.data(),
        res_payload.size());
  } else {
    return conn->sendErrorFrame(rc.message());
  }
}

} // namespace native_transport
} // namespace eventql

