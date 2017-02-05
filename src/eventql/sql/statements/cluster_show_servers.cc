
/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
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
#include "eventql/sql/statements/cluster_show_servers.h"
#include "eventql/sql/transaction.h"

namespace csql {

const std::vector<std::pair<std::string, SType>> ClusterShowServersExpression::kOutputColumns = {
  { "name", SType::STRING },
  { "status", SType::STRING },
  { "listenaddr", SType::STRING },
  { "buildinfo", SType::STRING },
  { "load", SType::STRING },
  { "disk_used", SType::STRING },
  { "disk_free", SType::STRING },
  { "partitions", SType::STRING }
};

ClusterShowServersExpression::ClusterShowServersExpression(
    Transaction* txn) :
    SimpleTableExpression(kOutputColumns),
    txn_(txn) {}

ReturnCode ClusterShowServersExpression::execute() {
  auto rc = txn_->getTableProvider()->listServers(
      [this] (const eventql::ServerConfig& server) {
    const auto& sstats = server.server_stats();
    auto partitions = StringUtil::format(
        "$0/$1",
        sstats.has_partitions_loaded() ?
            StringUtil::toString(sstats.partitions_loaded()) :
            "-",
        sstats.has_partitions_assigned() ?
            StringUtil::toString(sstats.partitions_assigned()) :
            "-");
    auto disk_available = sstats.has_disk_available() ?
        StringUtil::format("$0MB", sstats.disk_available() / 0x100000) :
        "-";
    auto disk_used = sstats.has_disk_used() ?
        StringUtil::format("$0MB", sstats.disk_used() / 0x100000) :
        "-";
    auto load = sstats.has_load_factor() ?
        StringUtil::toString(sstats.load_factor()) :
        "-";

    std::vector<SValue> row;
    row.emplace_back(SValue::newString(server.server_id())); //server name
    row.emplace_back(SValue::newString(ServerStatus_Name(server.server_status()))); //status
    row.emplace_back(SValue::newString(sstats.buildinfo())); //buildinfo
    row.emplace_back(SValue::newString(server.server_addr())); //listen_addr
    row.emplace_back(SValue::newString(load)); //load
    row.emplace_back(SValue::newString(disk_used));
    row.emplace_back(SValue::newString(disk_available));
    row.emplace_back(SValue::newString(partitions));
    addRow(row.data());
  });

  if (!rc.isSuccess()) {
    return rc;
  }

  return ReturnCode::success();
}

} // namespace csql

