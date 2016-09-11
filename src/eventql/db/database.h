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

namespace csql {
class Runtime;
}

namespace eventql {
class ProcessConfig;
class Session;
class PartitionMap;
class FileTracker;
class ConfigDirectory;
class ReplicationWorker;
class LSMTableIndexCache;
class MetadataStore;
class InternalAuth;
class ClientAuth;
class SQLService;
class TableService;
class MapReduceService;
class MetadataService;

struct DatabaseContext {
  ProcessConfig* config;
  PartitionMap* partition_map;
  FileTracker* file_tracker;
  ConfigDirectory* config_directory;
  ReplicationWorker* replication_worker;
  LSMTableIndexCache* lsm_index_cache;
  MetadataStore* metadata_store;
  InternalAuth* internal_auth;
  ClientAuth* client_auth;
  csql::Runtime* sql_runtime;
  SQLService* sql_service;
  TableService* table_service;
  MapReduceService* mapreduce_service;
  MetadataService* metadata_service;
  ProcessConfig* process_config;
};

class Database {
public:

  static Database* newDatabase(ProcessConfig* process_config);
  virtual ~Database() = default;

  virtual ReturnCode start() = 0;
  virtual void shutdown() = 0;

  /**
   * Start a thread from which database operations can be executed
   */
  virtual void startThread(std::function<void(Session* session)> entrypoint) = 0;

  /**
   * Get session for the current thread
   */
  virtual Session* getSession() const = 0;

  /**
   * Get the process config
   */
  virtual const ProcessConfig* getConfig() const = 0;

};

} // namespace eventql

