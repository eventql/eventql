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
#pragma once
#include "eventql/eventql.h"
#include "eventql/db/TableConfig.pb.h"
#include "eventql/util/protobuf/msg.h"
#include "eventql/util/SHA1.h"
#include "eventql/util/status.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/io/outputstream.h"

namespace eventql {

class MetadataFile : public RefCounted {
public:

  static const uint32_t kBinaryFormatVersion = 2;

  struct PartitionPlacement {
    String server_id;
    uint64_t placement_id;
  };

  struct PartitionMapEntry {
    PartitionMapEntry();

    String begin;
    SHA1Hash partition_id;
    Vector<PartitionPlacement> servers;
    Vector<PartitionPlacement> servers_joining;
    Vector<PartitionPlacement> servers_leaving;
    bool splitting;
    String split_point;
    SHA1Hash split_partition_id_low;
    SHA1Hash split_partition_id_high;
    Vector<PartitionPlacement> split_servers_low;
    Vector<PartitionPlacement> split_servers_high;
  };

  using PartitionMapIter =
      Vector<MetadataFile::PartitionMapEntry>::const_iterator;

  MetadataFile();
  MetadataFile(
      const SHA1Hash& transaction_id,
      uint64_t transaction_seq,
      KeyspaceType keyspace_type,
      const Vector<PartitionMapEntry>& partition_map);

  const SHA1Hash& getTransactionID() const;
  uint64_t getSequenceNumber() const;

  KeyspaceType getKeyspaceType() const;

  const Vector<PartitionMapEntry>& getPartitionMap() const;
  PartitionMapIter getPartitionMapBegin() const;
  PartitionMapIter getPartitionMapEnd() const;

  /**
   * Returns an iterator to the partition that handles the keyrange which
   * includes the provided key
   */
  PartitionMapIter getPartitionMapAt(const String& key) const;

  /**
   * Returns an iterator to the first partition that contains keys from the range
   * [begin, end)
   */
  PartitionMapIter getPartitionMapRangeBegin(const String& begin) const;

  /**
   * Returns an iterator to the first partition that does not contain keys from
   * the range  [begin, end)
   */
  PartitionMapIter getPartitionMapRangeEnd(const String& end) const;

  /**
   * Compare two keys, returns -1 if a < b, 0 if a == b and 1 if a > b
   */
  int compareKeys(const String& a, const String& b) const;

  Status decode(InputStream* is);
  Status encode(OutputStream* os) const;

  Status computeChecksum(SHA1Hash* checksum) const;

protected:
  SHA1Hash transaction_id_;
  uint64_t transaction_seq_;
  KeyspaceType keyspace_type_;
  Vector<PartitionMapEntry> partition_map_;
};

/**
 * Compare two keys, returns -1 if a < b, 0 if a == b and 1 if a > b
 */
int comparePartitionKeys(
    KeyspaceType keyspace_type,
    const String& a,
    const String& b);

String encodePartitionKey(
    KeyspaceType keyspace_type,
    const String& key);

String decodePartitionKey(
    KeyspaceType keyspace_type,
    const String& key);

} // namespace eventql
