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
#include "eventql/db/metadata_operation.h"
#include "eventql/util/random.h"
#include "eventql/util/logging.h"

namespace eventql {

MetadataOperation::MetadataOperation() {}

MetadataOperation::MetadataOperation(
    const String& db_namespace,
    const String& table_id,
    MetadataOperationType type,
    const SHA1Hash& input_txid,
    const SHA1Hash& output_txid,
    const Buffer& opdata) {
  data_.set_db_namespace(db_namespace);
  data_.set_table_id(table_id);
  data_.set_input_txid(input_txid.data(), input_txid.size());
  data_.set_output_txid(output_txid.data(), input_txid.size());
  data_.set_optype(type);
  data_.set_opdata(opdata.data(), opdata.size());
}

SHA1Hash MetadataOperation::getInputTransactionID() const {
  return SHA1Hash(data_.input_txid().data(), data_.input_txid().size());
}

SHA1Hash MetadataOperation::getOutputTransactionID() const {
  return SHA1Hash(data_.output_txid().data(), data_.output_txid().size());
}

Status MetadataOperation::decode(InputStream* is) {
  auto len = is->readVarUInt();
  Buffer buf(len);
  is->readNextBytes(buf.data(), buf.size());
  msg::decode(buf, &data_);
  return Status::success();
}

Status MetadataOperation::encode(OutputStream* os) const {
  auto buf = msg::encode(data_);
  os->appendVarUInt(buf->size());
  os->write((const char*) buf->data(), buf->size());
  return Status::success();
}

Status MetadataOperation::perform(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  logTrace("evqld", "Performing metadata operation: $0", data_.DebugString());

  switch (data_.optype()) {
    case METAOP_REMOVE_DEAD_SERVERS:
      return performRemoveDeadServers(input, output);
    case METAOP_SPLIT_PARTITION:
      return performSplitPartition(input, output);
    case METAOP_FINALIZE_SPLIT:
      return performFinalizeSplit(input, output);
    case METAOP_CREATE_PARTITION:
      return performCreatePartition(input, output);
    case METAOP_JOIN_SERVERS:
      return performJoinServers(input, output);
    case METAOP_FINALIZE_JOIN:
      return performFinalizeJoin(input, output);
    default:
      return Status(eIllegalArgumentError, "invalid metadata operation type");
  }
}

static void removeServers(
    const Set<String>& servers,
    Vector<MetadataFile::PartitionPlacement>* server_list) {
  auto iter = server_list->begin();
  while (iter != server_list->end()) {
    if (servers.count(iter->server_id) > 0) {
      iter = server_list->erase(iter);
    } else {
      ++iter;
    }
  }
}

Status MetadataOperation::performRemoveDeadServers(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  auto opdata = msg::decode<RemoveDeadServersOperation>(
      data_.opdata().data(),
      data_.opdata().size());

  Set<String> dead_server_ids;
  for (const auto& s : opdata.server_ids()) {
    dead_server_ids.emplace(s);
  }

  auto pmap = input.getPartitionMap();
  for (auto& p : pmap) {
    removeServers(dead_server_ids, &p.servers);
    removeServers(dead_server_ids, &p.servers_joining);
    removeServers(dead_server_ids, &p.servers_leaving);
    removeServers(dead_server_ids, &p.split_servers_low);
    removeServers(dead_server_ids, &p.split_servers_high);
  }

  *output = pmap;
  return Status::success();
}

Status MetadataOperation::performSplitPartition(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  if (input.hasUserDefinedPartitions()) {
    return Status(eIllegalArgumentError, "can't split user defined partitions");
  }

  auto opdata = msg::decode<SplitPartitionOperation>(
      data_.opdata().data(),
      data_.opdata().size());

  SHA1Hash partition_id(
      opdata.partition_id().data(),
      opdata.partition_id().size());

  bool success = false;
  auto pmap = input.getPartitionMap();
  auto iter = pmap.begin();
  for (; iter != pmap.end(); ++iter) {
    if (iter->partition_id != partition_id) {
      continue;
    }

    String iter_end;
    if (input.hasFinitePartitions() & MFILE_FINITE) {
      iter_end = iter->end;
    } else {
      auto iter_next = iter + 1;
      if (iter_next != pmap.end()) {
        iter_end = iter_next->begin;
      }
    }

    if (iter->splitting) {
      return Status(eIllegalStateError, "partition is already splitting");
    }

    if (!iter->begin.empty() &&
        input.compareKeys(opdata.split_point(), iter->begin) < 0) {
      return Status(eIllegalArgumentError, "split point is out of range");
    }

    if (!iter_end.empty() &&
        input.compareKeys(opdata.split_point(), iter_end) >= 0) {
      return Status(eIllegalArgumentError, "split point is out of range");
    }

    if (opdata.split_servers_low().size() == 0 ||
        opdata.split_servers_high().size() == 0) {
      return Status(eIllegalArgumentError, "split server list can't be empty");
    }

    if (opdata.finalize_immediately()) {
      MetadataFile::PartitionMapEntry lower_split;
      {
        lower_split.begin = iter->begin;
        lower_split.splitting = false;
        lower_split.partition_id = SHA1Hash(
            opdata.split_partition_id_low().data(),
            opdata.split_partition_id_low().size());

        if (input.hasFinitePartitions()) {
          lower_split.end = opdata.split_point();
        }

        for (const auto& s : opdata.split_servers_low()) {
          MetadataFile::PartitionPlacement p;
          p.server_id = s;
          p.placement_id = opdata.placement_id();
          lower_split.servers.emplace_back(p);
        }
      }

      MetadataFile::PartitionMapEntry higher_split;
      {
        higher_split.begin = opdata.split_point();
        higher_split.splitting = false;
        higher_split.partition_id = SHA1Hash(
            opdata.split_partition_id_high().data(),
            opdata.split_partition_id_high().size());

        if (input.hasFinitePartitions()) {
          higher_split.end = iter->end;
        }

        for (const auto& s : opdata.split_servers_high()) {
          MetadataFile::PartitionPlacement p;
          p.server_id = s;
          p.placement_id = opdata.placement_id();
          higher_split.servers.emplace_back(p);
        }
      }

      iter = pmap.erase(iter);
      iter = pmap.insert(iter, higher_split);
      iter = pmap.insert(iter, lower_split);
    } else {
      iter->splitting = true;
      iter->split_point = opdata.split_point();
      iter->split_partition_id_low = SHA1Hash(
          opdata.split_partition_id_low().data(),
          opdata.split_partition_id_low().size());
      iter->split_partition_id_high = SHA1Hash(
          opdata.split_partition_id_high().data(),
          opdata.split_partition_id_high().size());

      for (const auto& s : opdata.split_servers_low()) {
        MetadataFile::PartitionPlacement p;
        p.server_id = s;
        p.placement_id = opdata.placement_id();
        iter->split_servers_low.emplace_back(p);
      }

      for (const auto& s : opdata.split_servers_high()) {
        MetadataFile::PartitionPlacement p;
        p.server_id = s;
        p.placement_id = opdata.placement_id();
        iter->split_servers_high.emplace_back(p);
      }
    }

    success = true;
    break;
  }

  if (success) {
    *output = pmap;
    return Status::success();
  } else {
    return Status(eNotFoundError, "partition not found");
  }
}

Status MetadataOperation::performFinalizeSplit(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  auto opdata = msg::decode<FinalizeSplitOperation>(
      data_.opdata().data(),
      data_.opdata().size());

  SHA1Hash partition_id(
      opdata.partition_id().data(),
      opdata.partition_id().size());

  bool success = false;
  auto pmap = input.getPartitionMap();
  auto iter = pmap.begin();
  for (; iter != pmap.end(); ++iter) {
    if (iter->partition_id != partition_id) {
      continue;
    }

    if (!iter->splitting) {
      return Status(eIllegalStateError, "partition is not splitting");
    }

    MetadataFile::PartitionMapEntry lower_split;
    MetadataFile::PartitionMapEntry higher_split;

    {
      lower_split.partition_id = iter->split_partition_id_low;
      lower_split.splitting = false;
      lower_split.begin = iter->begin;
      if (input.hasFinitePartitions()) {
        lower_split.end = iter->split_point;
      }
      for (const auto& s : iter->split_servers_low) {
        lower_split.servers.emplace_back(s);
      }
    }

    {
      higher_split.partition_id = iter->split_partition_id_high;
      higher_split.begin = iter->split_point;
      higher_split.splitting = false;
      if (input.hasFinitePartitions()) {
        higher_split.end = iter->end;
      }
      for (const auto& s : iter->split_servers_high) {
        higher_split.servers.emplace_back(s);
      }
    }

    iter = pmap.erase(iter);
    iter = pmap.insert(iter, higher_split);
    iter = pmap.insert(iter, lower_split);

    success = true;
    break;
  }

  if (success) {
    *output = pmap;
    return Status::success();
  } else {
    return Status(eNotFoundError, "partition not found");
  }
}

Status MetadataOperation::performCreatePartition(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  if (!input.hasFinitePartitions()) {
    return Status(eIllegalArgumentError, "partitions not finite");
  }

  auto opdata = msg::decode<CreatePartitionOperation>(
      data_.opdata().data(),
      data_.opdata().size());

  SHA1Hash new_partition_id(
      opdata.partition_id().data(),
      opdata.partition_id().size());

  MetadataFile::PartitionMapEntry new_entry;
  new_entry.partition_id = new_partition_id;
  new_entry.splitting = false;
  new_entry.begin = opdata.begin();
  new_entry.end = opdata.end();
  for (const auto& s : opdata.servers()) {
    MetadataFile::PartitionPlacement p;
    p.server_id = s;
    p.placement_id = opdata.placement_id();
    new_entry.servers.emplace_back(p);
  }

  auto pmap = input.getPartitionMap();
  auto iter = pmap.begin();
  while (iter != pmap.end()) {
    if (input.compareKeys(iter->begin, new_entry.end) >= 0) {
      break;
    } else {
      ++iter;
    }
  }

  if (iter != pmap.begin()) {
    auto prev = iter - 1;
    if (input.compareKeys(prev->end, new_entry.begin) > 0) {
      return Status(eIllegalArgumentError, "overlapping partitions");
    }
  }

  iter = pmap.insert(iter, new_entry);

  *output = pmap;
  return Status::success();
}

static bool hasServer(
    const MetadataFile::PartitionMapEntry& e,
    const String& server_id) {
  for (const auto& e_ : e.servers) {
    if (e_.server_id == server_id) {
      return true;
    }
  }

  for (const auto& e_ : e.servers_joining) {
    if (e_.server_id == server_id) {
      return true;
    }
  }

  for (const auto& e_ : e.servers_leaving) {
    if (e_.server_id == server_id) {
      return true;
    }
  }

  return false;
}

Status MetadataOperation::performJoinServers(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  auto opdata = msg::decode<JoinServersOperation>(
      data_.opdata().data(),
      data_.opdata().size());

  HashMap<SHA1Hash, Vector<JoinServerOperation>> ops;
  for (const auto& o : opdata.ops()) {
    SHA1Hash partition_id(
        o.partition_id().data(),
        o.partition_id().size());

    ops[partition_id].emplace_back(o);
  }

  auto pmap  = input.getPartitionMap();
  for (auto& p : pmap) {
    for (const auto& o : ops[p.partition_id]) {
      if (hasServer(p, o.server_id())) {
        return Status(
            eIllegalArgumentError,
            "server already exists in server list");
      }

      MetadataFile::PartitionPlacement s;
      s.server_id = o.server_id();
      s.placement_id = o.placement_id();
      p.servers_joining.emplace_back(s);
    }
  }

  *output = pmap;
  return Status::success();
}

Status MetadataOperation::performFinalizeJoin(
    const MetadataFile& input,
    Vector<MetadataFile::PartitionMapEntry>* output) const {
  auto opdata = msg::decode<FinalizeJoinOperation>(
      data_.opdata().data(),
      data_.opdata().size());

  SHA1Hash partition_id(
      opdata.partition_id().data(),
      opdata.partition_id().size());

  bool success = false;
  auto pmap  = input.getPartitionMap();
  for (auto& p : pmap) {
    if (p.partition_id != partition_id) {
      continue;
    }

    bool server_found = false;
    for (auto s = p.servers_joining.begin(); s != p.servers_joining.end(); ) {
      if (s->server_id == opdata.server_id() &&
          s->placement_id == opdata.placement_id()) {
        server_found = true;
        s = p.servers_joining.erase(s);
      } else {
        ++s;
      }
    }

    if (!server_found) {
      return Status(
          eIllegalArgumentError,
          "server not included in join list");
    }

    MetadataFile::PartitionPlacement s;
    s.server_id = opdata.server_id();
    s.placement_id = opdata.placement_id();
    p.servers.emplace_back(s);
    success = true;
    break;
  }

  if (success) {
    *output = pmap;
    return Status::success();
  } else {
    return Status(eNotFoundError, "partition join not found");
  }
}

} // namespace eventql

