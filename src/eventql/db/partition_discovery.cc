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

  bool is_active_partition = false;
  bool is_active_server = false;
  bool is_joining_server = false;
  bool is_leaving_server = false;
  for (const auto& p : file->getPartitionMap()) {
    if (p.partition_id == req_partition_id) {
      is_active_partition = true;

      for (const auto& s : p.servers) {
        if (s.server_id == request.requester_id()) {
          is_active_server = true;
          break;
        }
      }

      for (const auto& s : p.servers_joining) {
        if (s.server_id == request.requester_id()) {
          is_joining_server = true;
          break;
        }
      }

      for (const auto& s : p.servers_leaving) {
        if (s.server_id == request.requester_id()) {
          is_leaving_server = true;
          break;
        }
      }

      break;
    }
  }

  if (is_active_partition) {
    if (is_active_server) {
      response->set_code(PDISCOVERY_SERVE);
    } else if (is_joining_server) {
      response->set_code(PDISCOVERY_LOAD);
    } else {
      response->set_code(PDISCOVERY_UNLOAD);
    }
  } else {
    response->set_code(PDISCOVERY_UNLOAD);
  }

  return Status::success();
}

} // namespace eventql

