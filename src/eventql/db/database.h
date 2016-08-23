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
#include <eventql/eventql.h>
#include <eventql/util/return_code.h>
#include <eventql/util/autoref.h>
#include "eventql/util/io/FileLock.h"

#include "eventql/eventql.h"

namespace csql {
  class QueryCache;
  class Runtime;
  class SymbolTable;
}

namespace eventql {
class GarbageCollector;
class FileTracker;
class ClientAuth;
class InternalAuth;
class ProcessConfig;
class MetadataService;
class MetadataStore;
class ConfigDirectory;
struct ServerCfg;
class PartitionMap;
class TableService;
class ReplicationWorker;
class CompactionWorker;
class MetadataReplication;
class SQLService;
class MapReduceService;
class Leader;
class Session;

class Database {
public:

  Database(ProcessConfig* process_config);

  ReturnCode start();
  void shutdown();

  /**
   * Create a new database connection context
   */
  std::unique_ptr<Session> createContext();

  /**
   * Start a thread from which database operations can be executed
   */
  void startThread(
      Session* context,
      std::function<void()> entrypoint);

protected:
  ProcessConfig* cfg_;
  std::unique_ptr<FileLock> server_lock_;
  std::unique_ptr<ServerCfg> server_cfg_;
  std::unique_ptr<FileTracker> file_tracker_;
  std::unique_ptr<GarbageCollector> garbage_collector_;
  std::unique_ptr<ConfigDirectory> config_dir_;
  std::unique_ptr<ClientAuth> client_auth_;
  std::unique_ptr<InternalAuth> internal_auth_;
  std::unique_ptr<MetadataStore> metadata_store_;
  std::unique_ptr<MetadataService> metadata_service_;
  std::unique_ptr<PartitionMap> partition_map_;
  std::unique_ptr<TableService> table_service_;
  std::unique_ptr<ReplicationWorker> replication_worker_;
  std::unique_ptr<CompactionWorker> compaction_worker_;
  std::unique_ptr<MetadataReplication> metadata_replication_;
  std::unique_ptr<csql::QueryCache> sql_query_cache_;
  RefPtr<csql::SymbolTable> sql_symbols_;
  RefPtr<csql::Runtime> sql_;
  std::unique_ptr<SQLService> sql_service_;
  std::unique_ptr<MapReduceService> mapreduce_service_;
  std::unique_ptr<Leader> leader_;
};

} // namespace tdsb

