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
#include "eventql/util/stdtypes.h"
#include "eventql/util/status.h"
#include "eventql/util/SHA1.h"
#include "eventql/db/metadata_file.h"
#include "eventql/db/metadata_store.h"
#include "eventql/db/metadata_cache.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/database.h"
#include "eventql/config/config_directory.h"

namespace eventql {

class MetadataService {
public:

  MetadataService(
      DatabaseContext* dbctx,
      MetadataStore* metadata_store,
      MetadataCache* cache);

  Status getMetadataFile(
      const String& ns,
      const String& table_name,
      RefPtr<MetadataFile>* file) const;

  Status getMetadataFile(
      const String& ns,
      const String& table_name,
      const SHA1Hash& transaction_id,
      RefPtr<MetadataFile>* file) const;

  Status createMetadataFile(
      const String& ns,
      const String& table_name,
      const MetadataFile& file) const;

  Status performMetadataOperation(
      const String& ns,
      const String& table_name,
      const MetadataOperation& op,
      MetadataOperationResult* result);

  Status discoverPartition(
      const PartitionDiscoveryRequest& request,
      PartitionDiscoveryResponse* response);

  Status listPartitions(
      const PartitionListRequest& request,
      PartitionListResponse* response);

  Status findPartition(
      const PartitionFindRequest& request,
      PartitionFindResponse* response);

  Status createPartition(
      const PartitionFindRequest& request,
      PartitionFindResponse* response);

protected:
  DatabaseContext* dbctx_;
  MetadataStore* metadata_store_;
  MetadataCache* cache_;
  std::mutex lockmap_mutex_;
  std::map<std::string, std::unique_ptr<std::mutex>> lockmap_;
};

} // namespace eventql

