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
#include <eventql/util/stdtypes.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/runtime/TableExpression.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/RemoteTSDBScanParams.pb.h>
#include <eventql/core/TSDBTableRef.h>
#include <eventql/core/ReplicationScheme.h>
#include <eventql/AnalyticsAuth.h>

using namespace stx;

namespace zbase {

class RemoteTSDBScan : public RefCounted {
public:

  RemoteTSDBScan(
      RefPtr<csql::SequentialScanNode> stmt,
      const String& customer,
      const TSDBTableRef& table_ref,
      ReplicationScheme* replication_scheme,
      AnalyticsAuth* auth);

  //void onInputsReady() override;
  //int nextRow(csql::SValue* out, int out_len) override;

  size_t rowsScanned() const;

protected:

  void executeOnHost(
      const RemoteTSDBScanParams& params,
      const InetAddr& host,
      Function<bool (int argc, const csql::SValue* argv)> fn);

  RefPtr<csql::SequentialScanNode> stmt_;
  String customer_;
  TSDBTableRef table_ref_;
  ReplicationScheme* replication_scheme_;
  AnalyticsAuth* auth_;
  size_t rows_scanned_;
};


} // namespace csql
