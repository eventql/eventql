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
#include "eventql/server/sql/pipelined_expression.h"
#include "eventql/server/sql/codec/binary_codec.h"


namespace eventql {

PipelinedExpression::PipelinedExpression(
    csql::Transaction* txn,
    csql::ExecutionContext* ctx,
    const String& db_namespace,
    InternalAuth* auth,
    size_t max_concurrency) :
    txn_(txn),
    ctx_(ctx),
    db_namespace_(db_namespace),
    auth_(auth),
    max_concurrency_(max_concurrency),
    num_columns_(0),
    eof_(false),
    error_(false),
    cancelled_(false),
    queries_started_(0),
    queries_finished_(0) {};

PipelinedExpression::~PipelinedExpression() {
  cancelled_ = true;
  cv_.notify_all();
  for (size_t i = 0; i < threads_.size(); ++i) {
    threads_[i].join();
  }
}

void PipelinedExpression::addLocalQuery(ScopedPtr<csql::TableExpression> expr) {
  num_columns_ = std::max(num_columns_, expr->getNumColumns());
  ctx_->incrementNumTasks();

  QuerySpec query_spec;
  query_spec.is_local = true;
  query_spec.expr = std::move(expr);
  queries_.emplace_back(std::move(query_spec));
}


void PipelinedExpression::addRemoteQuery(
    RefPtr<csql::TableExpressionNode> qtree,
    Vector<ReplicaRef> hosts) {
  num_columns_ = std::max(num_columns_, qtree->getNumComputedColumns());
  ctx_->incrementNumTasks();

  QuerySpec query_spec;
  query_spec.is_local = false;
  query_spec.qtree = qtree;
  query_spec.hosts = hosts;
  queries_.emplace_back(std::move(query_spec));
}

ScopedPtr<csql::ResultCursor> PipelinedExpression::execute() {
  size_t num_threads = std::min(queries_.size(), max_concurrency_);
  for (size_t i = 0; i < num_threads; ++i) {
    threads_.emplace_back(
        std::thread(std::bind(&PipelinedExpression::executeAsync, this)));
  }

  return mkScoped(
    new csql::DefaultResultCursor(
        num_columns_,
        std::bind(
            &PipelinedExpression::next,
            this,
            std::placeholders::_1,
            std::placeholders::_2)));
}

size_t PipelinedExpression::getNumColumns() const {
  return num_columns_;
}

void PipelinedExpression::executeAsync() {
  while (!cancelled_) {
    std::unique_lock<std::mutex> lk(mutex_);
    auto query_idx = queries_started_++;
    lk.unlock();

    if (query_idx >= queries_.size()) {
      break;
    }

    ctx_->incrementNumTasksRunning();

    const auto& query = queries_[query_idx];
    String error;
    try {
      if (query.is_local) {
        executeLocal(query);
      } else {
        executeRemote(query);
      }
    } catch (const StandardException& e) {
      error = e.what();
    }

    ctx_->incrementNumTasksCompleted();

    lk.lock();
    error_ = !error.empty();
    error_str_ += error;
    eof_ = ++queries_finished_ == queries_.size();
    cv_.notify_all();

    if (cancelled_ || error_) {
      break;
    }
  }
}

void PipelinedExpression::executeLocal(const QuerySpec& query) {
  auto cursor = query.expr->execute();
  Vector<csql::SValue> row(cursor->getNumColumns());
  while (cursor->next(row.data(), row.size())) {
    if (!returnRow(&*row.cbegin(), &*row.cend())) {
      return;
    }
  }
}

void PipelinedExpression::executeRemote(const QuerySpec& query) {
  size_t row_ctr = 0;

  for (size_t i = 0; i < query.hosts.size(); ++i) {
    try {
      logTrace(
          "eventql",
          "PipelinedExpression::executeOnHost running on $0",
          query.hosts[i].addr);

      executeOnHost(query.qtree, query.hosts[i].addr, &row_ctr);
      break;
    } catch (const StandardException& e) {
      logError(
          "eventql",
          e,
          "PipelinedExpression::executeOnHost failed @ $0",
          query.hosts[i].addr);

      if (row_ctr > 0 || i + 1 == query.hosts.size()) {
        throw e;
      }
    }
  }
}

void PipelinedExpression::executeOnHost(
    RefPtr<csql::TableExpressionNode> qtree,
    const String& host,
    size_t* row_ctr) {
  Buffer req_body;
  auto req_body_os = BufferOutputStream::fromBuffer(&req_body);
  csql::QueryTreeCoder qtree_coder(txn_);
  qtree_coder.encode(qtree.get(), req_body_os.get());

  bool error = false;
  csql::BinaryResultParser res_parser;

  res_parser.onRow([this, row_ctr] (int argc, const csql::SValue* argv) {
    if (returnRow(argv, argv + argc)) {
      ++(*row_ctr);
    } else {
      RAISE(kCancelledError);
    }
  });

  res_parser.onError([this, &error] (const String& error_str) {
    error = true;
    RAISE(kRuntimeError, error_str);
  });

  auto url = StringUtil::format("http://$0/api/v1/sql/execute_qtree", host);

  http::HTTPClient http_client(&z1stats()->http_client_stats);
  auto req = http::HTTPRequest::mkPost(url, req_body);
  auth_->signRequest(static_cast<Session*>(txn_->getUserData()), &req);

  auto res = http_client.executeRequest(
      req,
      http::StreamingResponseHandler::getFactory(
          std::bind(
              &csql::BinaryResultParser::parse,
              &res_parser,
              std::placeholders::_1,
              std::placeholders::_2)));

  if (!res_parser.eof() || error) {
    RAISE(kRuntimeError, "lost connection to upstream server");
  }

  if (res.statusCode() != 200) {
    RAISEF(
        kRuntimeError,
        "HTTP Error: $0",
        res.statusCode());
  }
}

bool PipelinedExpression::returnRow(
    const csql::SValue* begin,
    const csql::SValue* end) {
  std::unique_lock<std::mutex> lk(mutex_);

  while (!cancelled_ && !error_ && buf_.size() > kMaxBufferSize) {
    cv_.wait(lk);
  }

  if (cancelled_ || error_) {
    return false;
  }

  buf_.emplace_back(begin, end);
  cv_.notify_all();
  return true;
}

bool PipelinedExpression::next(csql::SValue* out_row, size_t out_row_len) {
  std::unique_lock<std::mutex> lk(mutex_);

  while (buf_.size() == 0 && !eof_ && !error_ && !cancelled_) {
    cv_.wait(lk);
  }

  if (cancelled_) {
    return false;
  }

  if (error_) {
    RAISEF(kIOError, "ERROR: $0", error_str_);
  }

  if (eof_ && buf_.size() == 0) {
    return false;
  }

  const auto& row = buf_.front();
  for (size_t i = 0; i < row.size() && i < out_row_len; ++i) {
    out_row[i] = row[i];
  }

  buf_.pop_front();
  cv_.notify_all();
  return true;
}



} // namespace eventql
