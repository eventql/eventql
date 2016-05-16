/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
    const String& db_namespace,
    AnalyticsAuth* auth) :
    txn_(txn),
    db_namespace_(db_namespace),
    auth_(auth),
    num_columns_(0),
    eof_(false),
    error_(false),
    cancelled_(false) {};

PipelinedExpression::~PipelinedExpression() {
  cancelled_ = true;
  cv_.notify_all();
  thread_.join();
}

void PipelinedExpression::addLocalQuery(ScopedPtr<csql::TableExpression> expr) {
  num_columns_ = std::max(num_columns_, expr->getNumColumns());

  queries_.emplace_back(QuerySpec {
    .is_local = true,
    .expr = std::move(expr)
  });
}


void PipelinedExpression::addRemoteQuery(
    RefPtr<csql::TableExpressionNode> qtree,
    Vector<ReplicaRef> hosts) {
  queries_.emplace_back(QuerySpec {
    .is_local = false,
    .qtree = qtree,
    .hosts = hosts
  });

  num_columns_ = std::max(num_columns_, qtree->numColumns());
}

ScopedPtr<csql::ResultCursor> PipelinedExpression::execute() {
  thread_ = std::thread(std::bind(&PipelinedExpression::executeAsync, this));

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
  Vector<String> errors;
  for (const auto& query : queries_) {
    for (const auto& host : query.hosts) {
      try {
        executeOnHost(query.qtree, host.addr);
        break;
      } catch (const StandardException& e) {
        logError(
            "eventql",
            e,
            "PipelinedExpression::executeOnHost failed @ $0",
            host.addr.hostAndPort());

        std::unique_lock<std::mutex> lk(mutex_);
        if (buf_ctr_ > 0) {
          errors.emplace_back(e.what());
          break;
        }
      }
    }

    if (errors.size() > 0) {
      break;
    }
  }

  std::unique_lock<std::mutex> lk(mutex_);
  error_ = errors.size() > 0;
  error_str_ = StringUtil::join(errors, ", ");
  eof_ = true;
  cv_.notify_all();
}

void PipelinedExpression::executeOnHost(
    RefPtr<csql::TableExpressionNode> qtree,
    const InetAddr& host) {
  Buffer req_body;
  auto req_body_os = BufferOutputStream::fromBuffer(&req_body);
  csql::QueryTreeCoder qtree_coder(txn_);
  qtree_coder.encode(qtree.get(), req_body_os.get());

  bool error = false;
  csql::BinaryResultParser res_parser;

  res_parser.onRow([this] (int argc, const csql::SValue* argv) {
    std::unique_lock<std::mutex> lk(mutex_);

    while (!cancelled_ && buf_.size() > kMaxBufferSize) {
      cv_.wait(lk);
    }

    if (cancelled_) {
      RAISE(kCancelledError);
    }

    buf_.emplace_back(argv, argv + argc);
    ++buf_ctr_;
    cv_.notify_all();
  });

  res_parser.onError([this, &error] (const String& error_str) {
    error = true;
    RAISE(kRuntimeError, error_str);
  });

  auto url = StringUtil::format(
      "http://$0/api/v1/sql/execute_qtree",
      host.ipAndPort());

  AnalyticsPrivileges privileges;
  privileges.set_allow_private_api_read_access(true);
  auto api_token = auth_->getPrivateAPIToken(db_namespace_, privileges);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client(&z1stats()->http_client_stats);
  auto req = http::HTTPRequest::mkPost(url, req_body, auth_headers);
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
  return true;
}



} // namespace eventql
