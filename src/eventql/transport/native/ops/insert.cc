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
#include "eventql/transport/native/frames/insert.h"
#include "eventql/auth/client_auth.h"
#include "eventql/util/csv.h"
#include "eventql/util/logging.h"
#include "eventql/util/wallclock.h"
#include "eventql/server/session.h"
#include <eventql/db/shredded_record.h>
#include <eventql/db/table_service.h>

namespace eventql {
namespace native_transport {

ReturnCode performOperation_INSERT_JSON(
    Session* session,
    TableService* table_service,
    InsertFrame* i_frame);

ReturnCode performOperation_INSERT_CSV(
    Session* session,
    TableService* table_service,
    InsertFrame* i_frame);

ReturnCode performOperation_INSERT(
    Database* database,
    NativeConnection* conn,
    const char* payload,
    size_t payload_size) {
  auto session = database->getSession();
  auto dbctx = session->getDatabaseContext();

  session->setHeartbeatCallback([conn] () -> ReturnCode {
    return conn->sendHeartbeatFrame();
  });

  InsertFrame i_frame;
  {
    MemoryInputStream payload_is(payload, payload_size);
    auto rc = i_frame.parseFrom(&payload_is);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  /* switch database */
  {
    auto rc = session->changeNamespace(i_frame.getDatabase());

    if (!rc.isSuccess()) {
      return conn->sendErrorFrame(rc.message());
    }
  }

  /* perform insert */
  auto rc = ReturnCode::success();
  switch (i_frame.getRecordEncoding()) {
    case EVQL_INSERT_CTYPE_JSON:
      rc = performOperation_INSERT_JSON(session, dbctx->table_service, &i_frame);
      break;
    case EVQL_INSERT_CTYPE_CSV:
      rc = performOperation_INSERT_CSV(session, dbctx->table_service, &i_frame);
      break;
    default:
      rc = ReturnCode::error("ERUNTIME", "invalid record encoding");
      break;
  }

  if (rc.isSuccess()) {
    conn->sendFrame(EVQL_OP_ACK, EVQL_ENDOFREQUEST, nullptr, 0);
  } else {
    conn->sendErrorFrame(rc.getMessage());
  }

  return ReturnCode::success();
}

ReturnCode performOperation_INSERT_JSON(
    Session* session,
    TableService* table_service,
    InsertFrame* i_frame) {
  std::vector<json::JSONObject> records;
  for (const auto& r : i_frame->getRecords()) {
    try {
      records.emplace_back(json::parseJSON(r));
    } catch (const std::exception& e) {
      return ReturnCode::exception(e);
    }
  }

  return table_service->insertRecords(
      session->getEffectiveNamespace(),
      i_frame->getTable(),
      &*records.begin(),
      &*records.end());
}

ReturnCode performOperation_INSERT_CSV(
    Session* session,
    TableService* table_service,
    InsertFrame* i_frame) {
  auto table_schema = table_service->tableSchema(
      session->getEffectiveNamespace(),
      i_frame->getTable());

  if (table_schema.isEmpty()) {
    return ReturnCode::error("ERUNTIME", "table not found");
  }

  std::vector<std::string> header;
  {
    auto rc = parseCSVLine(i_frame->getRecordEncodingInfo(), &header);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  std::vector<msg::DynamicMessage> records;
  for (const auto& r : i_frame->getRecords()) {
    std::vector<std::string> columns;
    auto rc = parseCSVLine(r, &columns);
    if (!rc.isSuccess()) {
      return rc;
    }

    if (columns.size() != header.size()) {
      return ReturnCode::error("ERUNTIME", "inconsistent number of columns");
    }

    records.emplace_back(table_schema.get());
    for (size_t i = 0; i < columns.size(); ++i) {
      records.back().addField(header[i], columns[i]);
    }
  }

  return table_service->insertRecords(
      session->getEffectiveNamespace(),
      i_frame->getTable(),
      &*records.begin(),
      &*records.end());
}

} // namespace native_transport
} // namespace eventql

