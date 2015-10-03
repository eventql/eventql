/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/qtree/SequentialScanNode.h>
#include <csql/runtime/TableExpression.h>
#include <csql/runtime/ValueExpression.h>
#include <zbase/RemoteTSDBScanParams.pb.h>
#include <zbase/core/TSDBTableRef.h>
#include <zbase/core/ReplicationScheme.h>

using namespace stx;

namespace zbase {

class RemoteTSDBScan : public csql::TableExpression {
public:

  RemoteTSDBScan(
      RefPtr<csql::SequentialScanNode> stmt,
      const TSDBTableRef& table_ref,
      ReplicationScheme* replication_scheme);

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

  void execute(
      csql::ExecutionContext* context,
      Function<bool (int argc, const csql::SValue* argv)> fn) override;

  size_t rowsScanned() const;

protected:

  void executeOnHost(
      const RemoteTSDBScanParams& params,
      const InetAddr& host);

  RefPtr<csql::SequentialScanNode> stmt_;
  TSDBTableRef table_ref_;
  ReplicationScheme* replication_scheme_;
  Vector<String> columns_;
  size_t rows_scanned_;
};


} // namespace csql
