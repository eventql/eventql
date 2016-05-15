/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/util/logging.h>
#include <eventql/util/thread/future.h>
#include <eventql/util/http/httpclient.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/core/RemoteTSDBScan.h>
#include <eventql/AnalyticsSession.pb.h>
#include <eventql/z1stats.h>

#include "eventql/eventql.h"

namespace eventql {

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
    rows_scanned_(0) {}

//int RemoteTSDBScan::nextRow(csql::SValue* out, int out_len) {
//  return -1;
//}
//void RemoteTSDBScan::onInputsReady() {
//    csql::ExecutionContext* context,
//    Function<bool (int argc, const csql::SValue* argv)> fn) {
//
//  RemoteTSDBScanParams params;
//
//  auto tbl_name = stmt_->tableName();
//  StringUtil::replaceAll(&tbl_name, "tsdb://remote/", "tsdb://localhost/");
//
//  params.set_table_name(tbl_name);
//  params.set_aggregation_strategy((uint64_t) stmt_->aggregationStrategy());
//
//  for (const auto& e : stmt_->selectList()) {
//    auto sl = params.add_select_list();
//    sl->set_expression(e->expression()->toSQL());
//    sl->set_alias(e->columnName());
//  }
//
//  auto where = stmt_->whereExpression();
//  if (!where.isEmpty()) {
//    params.set_where_expression(where.get()->toSQL());
//  }
//
//  auto replicas = replication_scheme_->replicaAddrsFor(
//      table_ref_.partition_key.get());
//
//  Vector<String> errors;
//  for (const auto& host : replicas) {
//    try {
//      executeOnHost(params, host, fn);
//      context->incrNumSubtasksCompleted(1);
//      return;
//    } catch (const StandardException& e) {
//      logError(
//          "eventql",
//          e,
//          "RemoteTSDBScan::executeOnHost failed @ $0",
//          host.hostAndPort());
//
//      errors.emplace_back(e.what());
//    }
//  }
//
//  RAISEF(
//      kRuntimeError,
//      "RemoteTSDBScan::execute failed: $0",
//      StringUtil::join(errors, ", "));
//}

void RemoteTSDBScan::executeOnHost(
    const RemoteTSDBScanParams& params,
    const InetAddr& host,
    Function<bool (int argc, const csql::SValue* argv)> fn) {
 // csql::BinaryResultParser res_parser;

 // bool reading = true;
 // res_parser.onRow([fn, &reading] (int argc, const csql::SValue* argv) {
 //   if (reading) {
 //     reading = fn(argc, argv);
 //   }
 // });

 // bool error = false;
 // res_parser.onError([&error] (const String& error_str) {
 //   error = true;
 //   RAISE(kRuntimeError, error_str);
 // });

 // auto url = StringUtil::format(
 //     "http://$0/api/v1/sql/scan_partition",
 //     host.ipAndPort());

 // AnalyticsPrivileges privileges;
 // privileges.set_allow_private_api_read_access(true);
 // auto api_token = auth_->getPrivateAPIToken(customer_, privileges);

 // http::HTTPMessage::HeaderList auth_headers;
 // auth_headers.emplace_back(
 //     "Authorization",
 //     StringUtil::format("Token $0", api_token));

 // http::HTTPClient http_client(&z1stats()->http_client_stats);
 // auto req_body = msg::encode(params);
 // auto req = http::HTTPRequest::mkPost(url, *req_body, auth_headers);
 // auto res = http_client.executeRequest(
 //     req,
 //     http::StreamingResponseHandler::getFactory(
 //         std::bind(
 //             &csql::BinaryResultParser::parse,
 //             &res_parser,
 //             std::placeholders::_1,
 //             std::placeholders::_2)));

 // if (!res_parser.eof() || error) {
 //   RAISE(kRuntimeError, "lost connection to upstream server");
 // }

 // if (res.statusCode() != 200) {
 //   RAISEF(
 //       kRuntimeError,
 //       "HTTP Error: $0",
 //       res.statusCode());
 // }
}

size_t RemoteTSDBScan::rowsScanned() const {
  return rows_scanned_;
}


} // namespace csql
