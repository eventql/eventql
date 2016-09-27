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
#pragma once
#include "eventql/eventql.h"
#include "eventql/util/status.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_file.h"
#include "eventql/db/metadata_cache.h"
#include "eventql/db/partition.h"
#include "eventql/config/config_directory.h"

namespace eventql {

struct MetadataClientLocks {
  std::mutex download_lock;
};

class MetadataClient {
public:

  MetadataClient(
      ConfigDirectory* cdir,
      ProcessConfig* config,
      MetadataStore* store,
      MetadataCache* cache,
      native_transport::TCPConnectionPool* conn_pool,
      net::DNSCache* dns_cache);

  Status fetchLatestMetadataFile(
      const String& ns,
      const String& table_id,
      RefPtr<MetadataFile>* file);

  Status fetchMetadataFile(
      const String& ns,
      const String& table_id,
      const SHA1Hash& txnid,
      RefPtr<MetadataFile>* file);

  Status listPartitions(
      const String& ns,
      const String& table_id,
      const KeyRange& keyrange,
      PartitionListResponse* res);

  Status findPartition(
      const String& ns,
      const String& table_id,
      const String& key,
      PartitionFindResponse* res);

  Status findOrCreatePartition(
      const String& ns,
      const String& table_id,
      const String& key,
      PartitionFindResponse* res);

protected:

  MetadataClientLocks* getAdvisoryLocks(
      const String& ns,
      const String& table_id);

  ReturnCode downloadMetadataFile(
      const String& ns,
      const String& table_id,
      const SHA1Hash& txnid,
      RefPtr<MetadataFile>* file);

  ConfigDirectory* cdir_;
  ProcessConfig* config_;
  MetadataCache* cache_;
  MetadataStore* store_;
  native_transport::TCPConnectionPool* conn_pool_;
  net::DNSCache* dns_cache_;
  std::mutex lockmap_mutex_;
  std::map<std::string, std::unique_ptr<MetadataClientLocks>> lockmap_;
};

} // namespace eventql
