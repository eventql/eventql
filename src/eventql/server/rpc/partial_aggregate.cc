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
#include "eventql/server/rpc/partial_aggregate.h"
#include "eventql/server/session.h"
#include "eventql/server/sql_service.h"
#include "eventql/sql/runtime/runtime.h"

namespace eventql {
namespace rpc {

PartialAggregationOperation::PartialAggregationOperation(Database* db) {
  auto dbctx = db->getSession()->getDatabaseContext();
  txn_ = dbctx->sql_service->startTransaction(db->getSession());
}

ReturnCode PartialAggregationOperation::parseFrom(const char* data, size_t len) {
  MemoryInputStream input(data, len);
  csql::QueryTreeCoder qtree_coder(txn_.get());
  try {
    qtree_ = qtree_coder.decode(&input);
  } catch (const std::exception& e) {
    return ReturnCode::error("ERUNTIME", e.what());
  }

  return ReturnCode::success();
}

ReturnCode PartialAggregationOperation::execute(OutputStream* os) {
  auto session = static_cast<Session*>(txn_->getUserData());
  auto dbctx = session->getDatabaseContext();
  if (session->getEffectiveNamespace().empty()) {
    return ReturnCode::error("EAUTH", "No database selected");
  }

  auto qplan = dbctx->sql_runtime->buildQueryPlan(txn_.get(), { qtree_ });
  return ReturnCode::success();
}

} // namespace rpc
} // namespace eventql

