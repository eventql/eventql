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
    eof_(false),
    error_(false),
    canceled_(false){};

RemoteExpression::~RemoteExpression() {
  canceled_ = false;
  thread_.join();
}

ScopedPtr<csql::ResultCursor> RemoteExpression::execute() {
  thread_ = std::thread(std::bind(&RemoteExpression::executeAsync, this));

  return mkScoped(
    new csql::DefaultResultCursor(
        qtree_->numColumns(),
        std::bind(
            &RemoteExpression::next,
            this,
            std::placeholders::_1,
            std::placeholders::_2)));
}

void RemoteExpression::executeAsync() {
}

bool RemoteExpression::next(csql::SValue* out_row, size_t out_row_len) {
  std::unique_lock<std::mutex> lk(mutex_);

  while (buf_.size() == 0 && !eof_ && !error_) {
    cv_.wait(lk);
  }

  if (error_) {
    RAISE(kIOError, "IO error");
  }

  if (eof_ && buf_.size() == 0) {
    return false;
  }

  const auto& row = buf_.front();
  for (size_t i = 0; i < row.size() && i < out_row_len; ++i) {
    out_row[i] = row[i];
  }

  buf_.pop_front();
  return true;
}


} // namespace zbase
