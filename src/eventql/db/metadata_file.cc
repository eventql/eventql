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
#include "eventql/db/metadata_file.h"

namespace eventql {

MetadataFile::MetadataFile() {}

MetadataFile::MetadataFile(
    const SHA1Hash& transaction_id,
    uint64_t transaction_seq,
    KeyspaceType keyspace_type,
    const Vector<PartitionMapEntry>& partition_map) :
    transaction_id_(transaction_id),
    transaction_seq_(transaction_seq),
    keyspace_type_(keyspace_type),
    partition_map_(partition_map) {}

const SHA1Hash& MetadataFile::getTransactionID() const {
  return transaction_id_;
}

uint64_t MetadataFile::getSequenceNumber() const {
  return transaction_seq_;
}

KeyspaceType MetadataFile::getKeyspaceType() const {
  return keyspace_type_;
}

const Vector<MetadataFile::PartitionMapEntry>&
    MetadataFile::getPartitionMap() const {
  return partition_map_;
}

MetadataFile::PartitionMapIter MetadataFile::getPartitionMapBegin() const {
  return partition_map_.begin();
}

MetadataFile::PartitionMapIter MetadataFile::getPartitionMapEnd() const {
  return partition_map_.end();
}

// fixme this should be a binary search
MetadataFile::PartitionMapIter MetadataFile::getPartitionMapAt(
    const String& key) const {
  auto iter = partition_map_.begin();

  while (iter < partition_map_.end()) {
    if (compareKeys(iter->begin, key) > 0) {
      break;
    } else {
      iter++;
    }
  }

  if (iter != partition_map_.begin()) {
    --iter;
  }

  return iter;
}

int MetadataFile::compareKeys(const String& a, const String& b) const {
  switch (keyspace_type_) {
    case KEYSPACE_STRING: {
      if (a < b) {
        return -1;
      } else if (a > b) {
        return 1;
      } else {
        return 0;
      }
    }

    case KEYSPACE_UINT64: {
      uint64_t a_uint = 0;
      uint64_t b_uint = 0;
      if (a.size() == sizeof(uint64_t)) {
        memcpy(&a_uint, a.data(), sizeof(uint64_t));
      }
      if (b.size() == sizeof(uint64_t)) {
        memcpy(&b_uint, b.data(), sizeof(uint64_t));
      }

      if (a_uint < b_uint) {
        return -1;
      } else if (a_uint > b_uint) {
        return 1;
      } else {
        return 0;
      }
    }
  }
}

static Status decodeServerList(
    Vector<MetadataFile::PartitionPlacement>* servers,
    InputStream* is) {
  auto n = is->readVarUInt();

  for (size_t i = 0; i < n; ++i) {
    MetadataFile::PartitionPlacement s;
    s.server_id = is->readLenencString();
    s.placement_id = is->readUInt64();
    servers->emplace_back(s);
  }

  return Status::success();
}

Status MetadataFile::decode(InputStream* is) {
  // file format version
  auto version = is->readUInt32();
  if (version != kBinaryFormatVersion) {
    return Status(eIOError, "invalid file format version");
  }

  // transaction id
  is->readNextBytes(
      (char*) transaction_id_.mutableData(),
      transaction_id_.size());

  // transaction seq
  transaction_seq_ = is->readUInt64();

  // keyspace type
  keyspace_type_ = static_cast<KeyspaceType>(is->readUInt8());

  // partition map
  auto pmap_size = is->readVarUInt();
  for (size_t i = 0; i < pmap_size; ++i) {
    PartitionMapEntry e;

    // begin
    e.begin = is->readLenencString();

    // partition id
    is->readNextBytes(
        (char*) e.partition_id.mutableData(),
        e.partition_id.size());

    // servers
    auto rc = decodeServerList(&e.servers, is);
    if (!rc.isSuccess()) {
      return rc;
    }

    // servers joining
    decodeServerList(&e.servers_joining, is);
    if (!rc.isSuccess()) {
      return rc;
    }

    // servers leaving
    decodeServerList(&e.servers_leaving, is);
    if (!rc.isSuccess()) {
      return rc;
    }

    // splitting
    e.splitting = is->readUInt8() > 0;
    if (e.splitting) {
      // split_point
      e.split_point = is->readLenencString();

      // split_servers_low
      decodeServerList(&e.split_servers_low, is);
      if (!rc.isSuccess()) {
        return rc;
      }

      // split_servers_high
      decodeServerList(&e.split_servers_high, is);
      if (!rc.isSuccess()) {
        return rc;
      }
    }

    partition_map_.emplace_back(e);
  }

  return Status::success();
}

static Status encodeServerList(
    const Vector<MetadataFile::PartitionPlacement>& servers,
    OutputStream* os) {
  os->appendVarUInt(servers.size());
  for (const auto& s : servers) {
    os->appendLenencString(s.server_id);
    os->appendUInt64(s.placement_id);
  }

  return Status::success();
}

Status MetadataFile::encode(OutputStream* os) const  {
  // file format version
  os->appendUInt32(kBinaryFormatVersion);

  // transaction id
  os->write((const char*) transaction_id_.data(), transaction_id_.size());

  // transaction seq
  os->appendUInt64(transaction_seq_);

  // keyspace type
  os->appendUInt8(static_cast<uint8_t>(keyspace_type_));

  // partition map
  os->appendVarUInt(partition_map_.size());
  for (const auto& p : partition_map_) {
    // begin
    os->appendLenencString(p.begin);

    // partition id
    os->write((const char*) p.partition_id.data(), p.partition_id.size());

    // servers
    auto rc = encodeServerList(p.servers, os);
    if (!rc.isSuccess()) {
      return rc;
    }

    // servers_joining
    rc = encodeServerList(p.servers_joining, os);
    if (!rc.isSuccess()) {
      return rc;
    }

    // servers_leaving
    rc = encodeServerList(p.servers_leaving, os);
    if (!rc.isSuccess()) {
      return rc;
    }

    // splitting
    os->appendUInt8(p.splitting);
    if (p.splitting) {
      // split_point
      os->appendLenencString(p.split_point);

      // split_servers_low
      rc = encodeServerList(p.split_servers_low, os);
      if (!rc.isSuccess()) {
        return rc;
      }

      // split_servers_high
      rc = encodeServerList(p.split_servers_high, os);
      if (!rc.isSuccess()) {
        return rc;
      }
    }
  }

  return Status::success();
}

} // namespace eventql

