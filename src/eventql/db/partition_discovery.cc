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
#include "eventql/db/partition_discovery.h"

namespace eventql {

Status PartitionDiscovery::discoverPartition(
    const MetadataFile* file,
    const PartitionDiscoveryRequest& request,
    PartitionDiscoveryResponse* response) {
  auto txid = file->getTransactionID();
  response->set_txnid(txid.data(), txid.size());
  response->set_txnseq(file->getSequenceNumber());
  response->set_code(PDISCOVERY_UNKNOWN);

  SHA1Hash req_partition_id(
      request.partition_id().data(),
      request.partition_id().size());

  auto add_repl_target = [&file, &request, &response] (
      MetadataFile::PartitionMapIter e,
      const MetadataFile::PartitionPlacement& s)  {
    auto t = response->add_replication_targets();
    t->set_server_id(s.server_id);
    t->set_placement_id(s.placement_id);
    t->set_partition_id(e->partition_id.data(), e->partition_id.size());
    t->set_keyrange_begin(e->begin);

    auto n = e + 1;
    if (n == file->getPartitionMapEnd()) {
      t->set_keyrange_end("");
    } else {
      t->set_keyrange_end(n->begin);
    }
  };

  auto iter = file->getPartitionMapRangeBegin(request.keyrange_begin());
  if (iter == file->getPartitionMapEnd()) {
    return Status(eIllegalStateError, "invalid partition map");
  }

  if (iter->partition_id == req_partition_id) {
    //valid partition
    // check the list of active servers
    for (const auto& s : iter->servers) {
      if (s.server_id == request.requester_id()) {
        // if we are in the active server list return SERVE
        response->set_code(PDISCOVERY_SERVE);
      } else {
        add_repl_target(iter, s);
      }
    }

    // check the list of joining servers
    for (const auto& s : iter->servers_joining) {
      if (s.server_id == request.requester_id()) {
        // if we are in the joining server list return LOAD
        response->set_code(PDISCOVERY_LOAD);
      } else {
        add_repl_target(iter, s);
      }
    }

    // check the list of leaving servers
    for (const auto& s : iter->servers_leaving) {
      if (s.server_id == request.requester_id()) {
        // if we are in the leaving server list return SERVE
        response->set_code(PDISCOVERY_SERVE);
      } else {
        add_repl_target(iter, s);
      }
    }

    // if we are neither in the active, joining or leaving server list, return
    // UNLOAD
    if (response->code() == PDISCOVERY_UNKNOWN) {
      response->set_code(PDISCOVERY_UNLOAD);
    }
  } else {
    //split or merged partition -> always return UNLOAD
    response->set_code(PDISCOVERY_UNLOAD);

    auto end = file->getPartitionMapRangeEnd(request.keyrange_end());
    for (; iter != end; ++iter) {
      for (const auto& s : iter->servers) {
        add_repl_target(iter, s);
      }
      for (const auto& s : iter->servers_joining) {
        add_repl_target(iter, s);
      }
      for (const auto& s : iter->servers_leaving) {
        add_repl_target(iter, s);
      }
    }
  }

  return Status::success();
}

} // namespace eventql

