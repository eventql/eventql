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
      RefPtr<csql::QueryTreeNode> qtree,
      Vector<ReplicaRef> hosts,
      AnalyticsAuth* auth);

  ScopedPtr<csql::ResultCursor> execute() override;

protected:

  csql::Transaction* txn_;
  RefPtr<csql::QueryTreeNode> qtree_;
  Vector<ReplicaRef> hosts_;
  AnalyticsAuth* auth_;
};

}

