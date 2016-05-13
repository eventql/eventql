/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/AnalyticsAuth.h>
#include <eventql/core/PartitionMap.h>

using namespace stx;

namespace zbase {

class RemoteExpression : public csql::TableExpression {
public:

  RemoteExpression(
      csql::Transaction* txn,
      RefPtr<csql::TableExpressionNode> qtree,
      Vector<ReplicaRef> hosts,
      AnalyticsAuth* auth);

  ~RemoteExpression();

  ScopedPtr<csql::ResultCursor> execute() override;

protected:

  void executeAsync();
  bool next(csql::SValue* out_row, size_t out_row_len);

  csql::Transaction* txn_;
  RefPtr<csql::TableExpressionNode> qtree_;
  Vector<ReplicaRef> hosts_;
  AnalyticsAuth* auth_;
  bool eof_;
  bool error_;
  std::atomic<bool> canceled_;
  std::mutex mutex_;
  std::condition_variable cv_;
  List<Vector<csql::SValue>> buf_;
  std::thread thread_;
};

}

