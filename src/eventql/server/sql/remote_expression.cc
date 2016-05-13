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
#include "eventql/server/sql/remote_expression.h"


namespace zbase {

RemoteExpression::RemoteExpression(
    csql::Transaction* txn,
    RefPtr<csql::QueryTreeNode> qtree,
    Vector<ReplicaRef> hosts,
    AnalyticsAuth* auth) :
    txn_(txn),
    qtree_(qtree),
    hosts_(hosts),
    auth_(auth) {};

ScopedPtr<csql::ResultCursor> RemoteExpression::execute() {
  RAISE(kNotYetImplementedError, "nyi remote expression");
}

} // namespace zbase
