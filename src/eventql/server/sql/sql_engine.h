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
#include <eventql/sql/runtime/runtime.h>
#include <eventql/db/TSDBService.h>
#include <eventql/AnalyticsAuth.h>

namespace eventql {
class TSDBService;

class SQLEngine {
public:

  static RefPtr<csql::TableProvider> tableProviderForNamespace(
      PartitionMap* partition_map,
      ReplicationScheme* replication_scheme,
      AnalyticsAuth* auth,
      const String& tsdb_namespace);
//
//  static RefPtr<csql::QueryTreeNode> rewriteQuery(
//      csql::Runtime* runtime,
//      PartitionMap* partition_map,
//      ReplicationScheme* replication_scheme,
//      AnalyticsAuth* auth,
//      const String& tsdb_namespace,
//      RefPtr<csql::QueryTreeNode> query);
//
//  static RefPtr<csql::ExecutionStrategy> getExecutionStrategy(
//      csql::Runtime* runtime,
//      PartitionMap* partition_map,
//      ReplicationScheme* replication_scheme,
//      AnalyticsAuth* auth,
//      const String& customer);
//
//protected:
//
//  // rewrite tbl.lastXXX to tbl WHERE time > x and time < x
//  static void rewriteTableTimeSuffix(
//      RefPtr<csql::QueryTreeNode> node);
};

void z1VersionExpr(sql_txn* ctx, int argc, csql::SValue* argv, csql::SValue* out);

}
