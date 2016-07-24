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
#include <thread>
#include "eventql/eventql.h"
#include <eventql/auth/internal_auth.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/partition_reader.h>
#include <eventql/server/sql/table_provider.h>
#include <eventql/server/sql/table_scan.h>
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/sql/qtree/SequentialScanNode.h>

namespace eventql {

class PipelinedExpression : public csql::TableExpression {
public:
  static const constexpr size_t kMaxBufferSize = 1024;

  PipelinedExpression(
      csql::Transaction* txn,
      csql::ExecutionContext* ctx,
      const String& db_namespace,
      InternalAuth* auth,
      size_t max_concurrency);

  ~PipelinedExpression();

  void addLocalQuery(ScopedPtr<csql::TableExpression> expr);

  void addRemoteQuery(
      RefPtr<csql::TableExpressionNode> qtree,
      Vector<ReplicaRef> hosts);

  ScopedPtr<csql::ResultCursor> execute() override;

  size_t getNumColumns() const override;

protected:

  struct QuerySpec {
    bool is_local;
    ScopedPtr<csql::TableExpression> expr;
    RefPtr<csql::TableExpressionNode> qtree;
    Vector<ReplicaRef> hosts;
  };

  void executeAsync();
  void executeOnHost(
      RefPtr<csql::TableExpressionNode> qtree,
      const String& host,
      size_t* row_ctr);

  void executeRemote(const QuerySpec& query);

  void executeLocal(const QuerySpec& query);

  bool returnRow(const csql::SValue* begin, const csql::SValue* end);

  bool next(csql::SValue* out_row, size_t out_row_len);

  csql::Transaction* txn_;
  csql::ExecutionContext* ctx_;
  String db_namespace_;
  Vector<QuerySpec> queries_;
  InternalAuth* auth_;
  size_t max_concurrency_;
  size_t num_columns_;
  bool eof_;
  bool error_;
  String error_str_;
  std::atomic<bool> cancelled_;
  std::mutex mutex_;
  std::condition_variable cv_;
  List<Vector<csql::SValue>> buf_;
  Vector<std::thread> threads_;
  size_t queries_started_;
  size_t queries_finished_;
};

}

