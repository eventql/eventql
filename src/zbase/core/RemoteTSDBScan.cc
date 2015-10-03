/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/logging.h>
#include <zbase/core/RemoteTSDBScan.h>

using namespace stx;

namespace zbase {

RemoteTSDBScan::RemoteTSDBScan(
    RefPtr<csql::SequentialScanNode> stmt,
    const TSDBTableRef& table_ref,
    ReplicationScheme* replication_scheme) :
    stmt_(stmt),
    table_ref_(table_ref),
    replication_scheme_(replication_scheme),
    rows_scanned_(0) {}

Vector<String> RemoteTSDBScan::columnNames() const {
  return columns_;
}

size_t RemoteTSDBScan::numColumns() const {
  return columns_.size();
}

void RemoteTSDBScan::execute(
    csql::ExecutionContext* context,
    Function<bool (int argc, const csql::SValue* argv)> fn) {

  RemoteTSDBScanParams params;

  params.set_table_name(stmt_->tableName());
  params.set_aggregation_strategy((uint64_t) stmt_->aggregationStrategy());

  for (const auto& e : stmt_->selectList()) {
    auto sl = params.add_select_list();
    sl->set_expression(e->expression()->toSQL());
    sl->set_alias(e->columnName());
  }

  auto where = stmt_->whereExpression();
  if (!where.isEmpty()) {
    params.set_where_expression(where.get()->toSQL());
  }

  auto replicas = replication_scheme_->replicaAddrsFor(
      table_ref_.partition_key.get());

  Vector<String> errors;
  for (const auto& host : replicas) {
    try {
      return executeOnHost(params, host);
    } catch (const StandardException& e) {
      logError(
          "zbase",
          e,
          "RemoteTSDBScan::executeOnHost failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "RemoteTSDBScan::execute failed: $0",
      StringUtil::join(errors, ", "));
}

void RemoteTSDBScan::executeOnHost(
    const RemoteTSDBScanParams& params,
    const InetAddr& host) {
  iputs("execute scan on $0 => $1", host.hostAndPort(), params.DebugString());
  RAISE(kNotYetImplementedError, "not yet implemented");
}

size_t RemoteTSDBScan::rowsScanned() const {
  return rows_scanned_;
}


} // namespace csql
