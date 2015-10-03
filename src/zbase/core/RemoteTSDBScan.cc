/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/logging.h>
#include <stx/thread/future.h>
#include <stx/http/httpclient.h>
#include <stx/protobuf/msg.h>
#include <zbase/core/RemoteTSDBScan.h>
#include <zbase/AnalyticsSession.pb.h>

using namespace stx;

namespace zbase {

RemoteTSDBScan::RemoteTSDBScan(
    RefPtr<csql::SequentialScanNode> stmt,
    const String& customer,
    const TSDBTableRef& table_ref,
    ReplicationScheme* replication_scheme,
    AnalyticsAuth* auth) :
    stmt_(stmt),
    customer_(customer),
    table_ref_(table_ref),
    replication_scheme_(replication_scheme),
    auth_(auth),
    rows_scanned_(0) {
  for (const auto& sl : stmt->selectList()) {
    columns_.emplace_back(sl->columnName());
  }
}

Vector<String> RemoteTSDBScan::columnNames() const {
  return columns_;
}

size_t RemoteTSDBScan::numColumns() const {
  return columns_.size();
}

void RemoteTSDBScan::execute(
    csql::ExecutionContext* context,
    Function<bool (int argc, const csql::SValue* argv)> fn) {

  RemoteTSDBScanParams params;

  params.set_table_name(stmt_->tableName());
  params.set_aggregation_strategy((uint64_t) stmt_->aggregationStrategy());

  for (const auto& e : stmt_->selectList()) {
    auto sl = params.add_select_list();
    sl->set_expression(e->expression()->toSQL());
    sl->set_alias(e->columnName());
  }

  auto where = stmt_->whereExpression();
  if (!where.isEmpty()) {
    params.set_where_expression(where.get()->toSQL());
  }

  auto replicas = replication_scheme_->replicaAddrsFor(
      table_ref_.partition_key.get());

  Vector<String> errors;
  for (const auto& host : replicas) {
    try {
      executeOnHost(params, host, fn);
      return;
    } catch (const StandardException& e) {
      logError(
          "zbase",
          e,
          "RemoteTSDBScan::executeOnHost failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "RemoteTSDBScan::execute failed: $0",
      StringUtil::join(errors, ", "));
}

void RemoteTSDBScan::executeOnHost(
    const RemoteTSDBScanParams& params,
  const InetAddr& host,
      Function<bool (int argc, const csql::SValue* argv)> fn) {
  Buffer buffer;
  bool eos = false;
  bool done = false;
  auto handler = [&buffer, &eos, &done, &fn] (const void* data, size_t size) {
    buffer.append(data, size);
    size_t consumed = 0;

    util::BinaryMessageReader reader(buffer.data(), buffer.size());
    while (reader.remaining() >= sizeof(uint64_t)) {
      auto rec_len = *reader.readUInt64();

      if (rec_len > reader.remaining()) {
        break;
      }

      if (rec_len == 0) {
        eos = true;
      } else {
        auto rec_data = reader.read(rec_len);

        if (!done) {
          MemoryInputStream is(rec_data, rec_len);
          Vector<csql::SValue> row;
          while (!is.eof()) {
            csql::SValue val;
            val.decode(&is);
            row.emplace_back(val);

            if (!fn(row.size(), row.data())) {
              done = true;
            }
          }
        }

        consumed = reader.position();
      }
    }

    Buffer remaining((char*) buffer.data() + consumed, buffer.size() - consumed);
    buffer.clear();
    buffer.append(remaining);
  };

  auto handler_factory = [&handler] (const Promise<http::HTTPResponse> promise)
      -> http::HTTPResponseFuture* {
    return new http::StreamingResponseFuture(promise, handler);
  };

  auto url = StringUtil::format(
      "http://$0/api/v1/sql/scan_partition",
      host.ipAndPort());

  AnalyticsPrivileges privileges;
  privileges.set_allow_private_api_read_access(true);
  auto api_token = auth_->getPrivateAPIToken(customer_, privileges);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client;
  auto req_body = msg::encode(params);
  auto req = http::HTTPRequest::mkPost(url, *req_body, auth_headers);
  auto res = http_client.executeRequest(req, handler_factory);
  handler(nullptr, 0);

  if (!eos) {
    RAISE(kRuntimeError, "unexpected EOF");
  }

  if (res.statusCode() != 200) {
    RAISEF(
        kRuntimeError,
        "received non-200 response: $0",
        res.body().toString());
  }
}

size_t RemoteTSDBScan::rowsScanned() const {
  return rows_scanned_;
}


} // namespace csql
