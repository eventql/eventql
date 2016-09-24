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
#include "eventql/config/config_directory.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/status.h>
#include <eventql/db/server_allocator.h>
#include <eventql/db/metadata_client.h>
#include <eventql/db/metadata_coordinator.h>

namespace eventql {

class Rebalance {
public:

  Rebalance(
      ConfigDirectory* cdir,
      ProcessConfig* config,
      ServerAllocator* server_alloc);

  Status runOnce();

protected:

  Status rebalanceTable(TableDefinition tbl_cfg);

  Status performMetadataOperation(
      TableDefinition* table_cfg,
      MetadataFile* metadata_file,
      MetadataOperationType optype,
      const Buffer& opdata);

  ConfigDirectory* cdir_;
  ProcessConfig* config_;
  ServerAllocator* server_alloc_;
  MetadataCoordinator metadata_coordinator_;
  MetadataClient metadata_client_;
  size_t replication_factor_;
  size_t metadata_replication_factor_;
  Set<String> all_servers_;
  Set<String> live_servers_;
  Set<String> leaving_servers_;
};

} // namespace eventql

