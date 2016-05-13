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
#include "eventql/server/sql/codec/binary_codec.h"


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
    cancelled_(false){};

RemoteExpression::~RemoteExpression() {
  cancelled_ = true;
  cv_.notify_all();
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
  Vector<String> errors;
  for (const auto& host : hosts_) {
    try {
      executeOnHost(host.addr);
      break;
    } catch (const StandardException& e) {
      logError(
          "zbase",
          e,
          "RemoteExpression::executeOnHost failed @ $0",
          host.addr.hostAndPort());

      errors.emplace_back(e.what());

      std::unique_lock<std::mutex> lk(mutex_);
      if (buf_ctr_ > 0) {
        break;
      }
    }
  }

  std::unique_lock<std::mutex> lk(mutex_);
  error_ = errors.size() > 0;
  error_str_ = StringUtil::join(errors, ", ");
  eof_ = true;
  cv_.notify_all();
}

void RemoteExpression::executeOnHost(const InetAddr& host) {
  Buffer req_body;
  auto req_body_os = BufferOutputStream::fromBuffer(&req_body);
  csql::QueryTreeCoder qtree_coder(txn_);
  qtree_coder.encode(qtree_.get(), req_body_os.get());

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
  auto api_token = auth_->getPrivateAPIToken("acme_corp", privileges); // FIXME

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


bool RemoteExpression::next(csql::SValue* out_row, size_t out_row_len) {
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



} // namespace zbase
