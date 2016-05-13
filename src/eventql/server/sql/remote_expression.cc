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
    RefPtr<csql::TableExpressionNode> qtree,
    Vector<ReplicaRef> hosts,
    AnalyticsAuth* auth) :
    txn_(txn),
    qtree_(qtree),
    hosts_(hosts),
    auth_(auth),
    complete_(false),
    error_(false),
    canceled_(false){};

ScopedPtr<csql::ResultCursor> RemoteExpression::execute() {
  return mkScoped(
    new csql::DefaultResultCursor(
        qtree_->numColumns(),
        std::bind(
            &RemoteExpression::next,
            this,
            std::placeholders::_1,
            std::placeholders::_2)));
}

bool RemoteExpression::next(csql::SValue* row, size_t row_len) {
  return false;
}


} // namespace zbase
