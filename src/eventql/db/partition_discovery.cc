/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
  if (request.lookup_by_id()) {
    return discoverPartitionByID(file, request, response);
  } else {
    return discoverPartitionByKeyRange(file, request, response);
  }
}

static void addReplicationTarget(
    PartitionDiscoveryResponse* response,
    const MetadataFile* file,
    MetadataFile::PartitionMapIter e,
    const MetadataFile::PartitionPlacement& s,
    bool is_joining) {
  auto t = response->add_replication_targets();
  t->set_server_id(s.server_id);
  t->set_placement_id(s.placement_id);
  t->set_partition_id(e->partition_id.data(), e->partition_id.size());
  t->set_keyrange_begin(e->begin);
  t->set_is_joining(is_joining);

  if (file->hasFinitePartitions()) {
    t->set_keyrange_end(e->end);
  } else if (file->hasUserDefinedPartitions()) {
    /* user defined partitions have no defined "end" */
  } else {
    auto n = e + 1;
    if (n == file->getPartitionMapEnd()) {
      t->set_keyrange_end("");
    } else {
      t->set_keyrange_end(n->begin);
    }
  }
}

static void addSplittingReplicationTargets(
    PartitionDiscoveryResponse* response,
    const MetadataFile* file,
    MetadataFile::PartitionMapIter e) {
  String e_end;
  if (file->hasFinitePartitions()) {
    e_end = e->end;
  } else if (file->hasUserDefinedPartitions()) {
    /* user defined partitions have no defined "end" */
  } else {
    auto n = e + 1;
    if (n != file->getPartitionMapEnd()) {
      e_end = n->begin;
    }
  }

  for (const auto& s : e->split_servers_low) {
    auto t = response->add_replication_targets();
    t->set_server_id(s.server_id);
    t->set_placement_id(s.placement_id);
    t->set_partition_id(
        e->split_partition_id_low.data(),
        e->split_partition_id_low.size());
    t->set_keyrange_begin(e->begin);
    t->set_keyrange_end(e->split_point);
  }

  for (const auto& s : e->split_servers_high) {
    auto t = response->add_replication_targets();
    t->set_server_id(s.server_id);
    t->set_placement_id(s.placement_id);
    t->set_partition_id(
        e->split_partition_id_high.data(),
        e->split_partition_id_high.size());
    t->set_keyrange_begin(e->split_point);
    t->set_keyrange_end(e_end);
  }
}

Status PartitionDiscovery::discoverPartitionByKeyRange(
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

  auto pmap_end = file->getPartitionMapEnd();
  auto iter = file->getPartitionMapRangeBegin(request.keyrange_begin());
  if (iter == pmap_end) {
    return Status(eIllegalStateError, "invalid key range requested");
  }

  if (iter->partition_id == req_partition_id) {
    //valid partition
    response->set_keyrange_begin(iter->begin);
    if (file->hasFinitePartitions()) {
      response->set_keyrange_end(iter->end);
    } else if (file->hasUserDefinedPartitions()) {
      /* user defined partitions have no defined "end" */
    } else if (iter + 1 != pmap_end) {
      response->set_keyrange_end(iter[1].begin);
    }

    // check the list of active servers
    for (const auto& s : iter->servers) {
      if (s.server_id == request.requester_id()) {
        // if we are in the active server list return SERVE
        response->set_code(PDISCOVERY_SERVE);
      } else {
        addReplicationTarget(response, file, iter, s, false);
      }
    }

    // check the list of joining servers
    for (const auto& s : iter->servers_joining) {
      if (s.server_id == request.requester_id()) {
        // if we are in the joining server list return LOAD
        response->set_code(PDISCOVERY_LOAD);
      } else {
        addReplicationTarget(response, file, iter, s, true);
      }
    }

    // check the list of leaving servers
    for (const auto& s : iter->servers_leaving) {
      if (s.server_id == request.requester_id()) {
        // if we are in the leaving server list return SERVE
        response->set_code(PDISCOVERY_SERVE);
      } else {
        addReplicationTarget(response, file, iter, s, false);
      }
    }

    if (iter->splitting) {
      addSplittingReplicationTargets(response, file, iter);
      response->set_is_splitting(true);
      response->add_split_partition_ids(
          iter->split_partition_id_low.data(),
          iter->split_partition_id_low.size());
      response->add_split_partition_ids(
          iter->split_partition_id_high.data(),
          iter->split_partition_id_high.size());
    }

    // if we are neither in the active, joining or leaving server list, return
    // UNLOAD
    if (response->code() == PDISCOVERY_UNKNOWN) {
      response->set_code(PDISCOVERY_UNLOAD);
    }
  } else if (iter->split_partition_id_low == req_partition_id) {
    // splitting, valid lower split range, always load
    response->set_code(PDISCOVERY_LOAD);
    response->set_keyrange_begin(iter->begin);
    response->set_keyrange_end(iter->split_point);

    for (const auto& s : iter->split_servers_low) {
      if (s.server_id == request.requester_id()) {
        continue;
      }

      auto t = response->add_replication_targets();
      t->set_server_id(s.server_id);
      t->set_placement_id(s.placement_id);
      t->set_partition_id(
          iter->split_partition_id_low.data(),
          iter->split_partition_id_low.size());

      t->set_keyrange_begin(iter->begin);
      t->set_keyrange_end(iter->split_point);
    }
  } else if (iter->split_partition_id_high == req_partition_id) {
    // splitting, higher split range, always load
    response->set_code(PDISCOVERY_LOAD);

    String iter_end;
    if (file->hasFinitePartitions()) {
      iter_end = iter->end;
    } else if (file->hasUserDefinedPartitions()) {
      /* user defined partitions have no defined "end" */
    } else {
      auto iter_next = iter + 1;
      if (iter_next != file->getPartitionMapEnd()) {
        iter_end = iter_next->begin;
      }
    }

    response->set_keyrange_begin(iter->split_point);
    response->set_keyrange_end(iter_end);

    for (const auto& s : iter->split_servers_high) {
      if (s.server_id == request.requester_id()) {
        continue;
      }

      auto t = response->add_replication_targets();
      t->set_server_id(s.server_id);
      t->set_placement_id(s.placement_id);
      t->set_partition_id(
          iter->split_partition_id_high.data(),
          iter->split_partition_id_high.size());

      t->set_keyrange_begin(iter->split_point);
      t->set_keyrange_end(iter_end);
    }
  } else {
    //split or merged partition -> always return UNLOAD
    response->set_code(PDISCOVERY_UNLOAD);

    auto end = file->getPartitionMapRangeEnd(request.keyrange_end());
    for (; iter != end; ++iter) {
      for (const auto& s : iter->servers) {
        addReplicationTarget(response, file, iter, s, false);
      }
      for (const auto& s : iter->servers_joining) {
        addReplicationTarget(response, file, iter, s, true);
      }
      for (const auto& s : iter->servers_leaving) {
        addReplicationTarget(response, file, iter, s, false);
      }
      if (iter->splitting) {
        addSplittingReplicationTargets(response, file, iter);
      }
    }
  }

  return Status::success();
}

Status PartitionDiscovery::discoverPartitionByID(
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

  auto iter = file->getPartitionMapBegin();
  auto pmap_end = file->getPartitionMapEnd();
  for (; iter != pmap_end; ++iter) {
    // found valid partition
    if (iter->partition_id == req_partition_id) {
      response->set_keyrange_begin(iter->begin);
      if (file->hasFinitePartitions()) {
        response->set_keyrange_end(iter->end);
      } else if (file->hasUserDefinedPartitions()) {
        /* user defined partitions have no defined "end" */
      } else if (iter + 1 != pmap_end) {
        response->set_keyrange_end(iter[1].begin);
      }

      // check the list of active servers
      for (const auto& s : iter->servers) {
        if (s.server_id == request.requester_id()) {
          // if we are in the active server list return SERVE
          response->set_code(PDISCOVERY_SERVE);
        } else {
          addReplicationTarget(response, file, iter, s, false);
        }
      }

      // check the list of joining servers
      for (const auto& s : iter->servers_joining) {
        if (s.server_id == request.requester_id()) {
          // if we are in the joining server list return LOAD
          response->set_code(PDISCOVERY_LOAD);
        } else {
          addReplicationTarget(response, file, iter, s, true);
        }
      }

      // check the list of leaving servers
      for (const auto& s : iter->servers_leaving) {
        if (s.server_id == request.requester_id()) {
          // if we are in the leaving server list return SERVE
          response->set_code(PDISCOVERY_SERVE);
        } else {
          addReplicationTarget(response, file, iter, s, false);
        }
      }

      break;
    }

    // found valid lower half of ongoing split
    if (iter->split_partition_id_low == req_partition_id) {
      response->set_code(PDISCOVERY_LOAD);
      response->set_keyrange_begin(iter->begin);
      response->set_keyrange_end(iter->split_point);

      for (const auto& s : iter->split_servers_low) {
        if (s.server_id == request.requester_id()) {
          continue;
        }

        auto t = response->add_replication_targets();
        t->set_server_id(s.server_id);
        t->set_placement_id(s.placement_id);
        t->set_partition_id(
            iter->split_partition_id_low.data(),
            iter->split_partition_id_low.size());

        t->set_keyrange_begin(iter->begin);
        t->set_keyrange_end(iter->split_point);
      }

      break;
    }

    // found valid upper half of ongoing split
    if (iter->split_partition_id_high == req_partition_id) {
      response->set_code(PDISCOVERY_LOAD);

      String iter_end;
      if (file->hasFinitePartitions()) {
        iter_end = iter->end;
      } else if (file->hasUserDefinedPartitions()) {
        /* user defined partitions have no defined "end" */
      } else {
        auto iter_next = iter + 1;
        if (iter_next != file->getPartitionMapEnd()) {
          iter_end = iter_next->begin;
        }
      }

      response->set_keyrange_begin(iter->split_point);
      response->set_keyrange_end(iter_end);

      for (const auto& s : iter->split_servers_high) {
        if (s.server_id == request.requester_id()) {
          continue;
        }

        auto t = response->add_replication_targets();
        t->set_server_id(s.server_id);
        t->set_placement_id(s.placement_id);
        t->set_partition_id(
            iter->split_partition_id_high.data(),
            iter->split_partition_id_high.size());

        t->set_keyrange_begin(iter->split_point);
        t->set_keyrange_end(iter_end);
      }

      break;
    }
  }

  return Status::success();
}

} // namespace eventql

