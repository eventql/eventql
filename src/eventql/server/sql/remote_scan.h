/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/runtime/TableExpression.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/RemoteTSDBScanParams.pb.h>
#include <eventql/db/TSDBTableRef.h>
#include <eventql/db/ReplicationScheme.h>
#include <eventql/AnalyticsAuth.h>

#include "eventql/eventql.h"

namespace eventql {

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
