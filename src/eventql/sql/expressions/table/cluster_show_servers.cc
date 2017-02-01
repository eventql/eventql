
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
#include "eventql/sql/expressions/table/cluster_show_servers.h"
#include "eventql/sql/transaction.h"

namespace csql {

ClusterShowServersExpression::ClusterShowServersExpression(
    Transaction* txn) :
    txn_(txn),
    counter_(0) {}

ReturnCode ClusterShowServersExpression::execute() {
  auto rc = txn_->getTableProvider()->listServers(
      [this] (const eventql::ServerConfig& server) {
    rows_.emplace_back(server);
  });

  if (!rc.isSuccess()) {
    return rc;
  }

  return ReturnCode::success();
}

ReturnCode ClusterShowServersExpression::nextBatch(
    size_t limit,
    SVector* columns,
    size_t* nrecords) {
  return ReturnCode::error("ERUNTIME", "ClusterShowServersExpression::nextBatch not yet implemented");
}

size_t ClusterShowServersExpression::getColumnCount() const {
  return kNumColumns;
}

SType ClusterShowServersExpression::getColumnType(size_t idx) const {
  assert(idx < kNumColumns);
  return SType::STRING;
}

bool ClusterShowServersExpression::next(SValue* row, size_t row_len) {
  if (counter_ < rows_.size()) {
    const auto& server = rows_[counter_];
    const auto& sstats = server.server_stats();
    switch (row_len) {
      default:
      case 8: {
        auto partitions = StringUtil::format(
            "$0/$1",
            sstats.has_partitions_loaded() ?
                StringUtil::toString(sstats.partitions_loaded()) :
                "-",
            sstats.has_partitions_assigned() ?
                StringUtil::toString(sstats.partitions_assigned()) :
                "-");
        row[7] = SValue::newString(partitions);
      }
      case 7: {
        auto disk_available = sstats.has_disk_available() ?
            StringUtil::format("$0MB", sstats.disk_available() / 0x100000) :
            "-";
        row[6] = SValue::newString(disk_available);
      }
      case 6: {
        auto disk_used = sstats.has_disk_used() ?
            StringUtil::format("$0MB", sstats.disk_used() / 0x100000) :
            "-";
        row[5] = SValue::newString(disk_used);
      }
      case 5: {
        auto load = sstats.has_load_factor() ?
            StringUtil::toString(sstats.load_factor()) :
            "-";
        row[4] = SValue::newString(load); //load
      }
      case 4:
        row[3] = SValue::newString(sstats.buildinfo()); //buildinfo
      case 3:
        row[2] = SValue::newString(server.server_addr()); //listen_addr
      case 2:
        row[1] = SValue::newString(ServerStatus_Name(server.server_status())); //status
      case 1:
        row[0] = SValue::newString(server.server_id()); //server name
      case 0:
        break;
    }

    ++counter_;
    return true;
  } else {
    return false;
  }
}

} //csql

