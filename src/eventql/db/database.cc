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
#include <eventql/db/database.h>
#include <eventql/util/autoref.h>
#include "eventql/util/io/FileLock.h"
#include "eventql/util/io/filerepository.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/random.h"
#include "eventql/util/assets.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/json/json.h"
#include "eventql/util/io/FileLock.h"
#include "eventql/util/stats/statsdagent.h"
#include "eventql/io/sstable/SSTableServlet.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "eventql/db/table_config.pb.h"
#include "eventql/db/table_service.h"
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/metadata_replication.h"
#include "eventql/db/metadata_service.h"
#include "eventql/db/replication_worker.h"
#include "eventql/db/tablet_index_cache.h"
#include "eventql/db/compaction_worker.h"
#include "eventql/db/garbage_collector.h"
#include "eventql/db/leader.h"
#include "eventql/db/monitor.h"
#include "eventql/db/database.h"
#include "eventql/db/server_allocator.h"
#include "eventql/sql/defaults.h"
#include "eventql/sql/runtime/query_cache.h"
#include "eventql/config/config_directory.h"
#include "eventql/config/config_directory_zookeeper.h"
#include "eventql/config/config_directory_standalone.h"
#include "eventql/server/sql/scheduler.h"
#include "eventql/server/sql/table_provider.h"
#include "eventql/server/listener.h"
#include "eventql/auth/client_auth.h"
#include "eventql/auth/client_auth_trust.h"
#include "eventql/auth/client_auth_legacy.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/auth/internal_auth_trust.h"
#include <jsapi.h>
#include "eventql/mapreduce/mapreduce_preludejs.cc"
#include "eventql/eventql.h"
#include "eventql/db/file_tracker.h"
#include <pthread.h>

namespace js {
void DisableExtraThreads();
}

namespace eventql {

class DatabaseImpl : public Database {
public:

  DatabaseImpl(ProcessConfig* process_config);
  ~DatabaseImpl();

  ReturnCode start() override;
  void shutdown() override;

  void startThread(std::function<void(Session* session)> entrypoint) override;
  Session* getSession() const override;
  const ProcessConfig* getConfig() const override;

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
  std::unique_ptr<MetadataCache> metadata_cache_;
  std::unique_ptr<MetadataService> metadata_service_;
  std::unique_ptr<MetadataClient> metadata_client_;
  std::unique_ptr<MetadataCoordinator> metadata_coordinator_;
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
  std::unique_ptr<Monitor> monitor_;
  std::unique_ptr<DatabaseContext> database_context_;
  std::unique_ptr<ServerAllocator> server_alloc_;
  std::unique_ptr<native_transport::TCPConnectionPool> connection_pool_;
  std::unique_ptr<net::DNSCache> dns_cache_;
  pthread_key_t local_session_;
};

Database* Database::newDatabase(ProcessConfig* process_config) {
  return new DatabaseImpl(process_config);
}

DatabaseImpl::DatabaseImpl(
    ProcessConfig* process_config) :
    cfg_(process_config) {
  pthread_key_create(&local_session_, NULL);
}

DatabaseImpl::~DatabaseImpl() {
  pthread_key_delete(local_session_);
}

ReturnCode DatabaseImpl::start() {
  /* check preconditions */
  if (cfg_->getString("cluster.allowed_hosts").isEmpty()) {
    return ReturnCode::error(
        "EARG",
        "cluster.allowed_hosts can't be empty "
        "(no server would be allowed to join)");
  }

  /* data directories */
  auto server_datadir = cfg_->getString("server.datadir").get();
  if (!FileUtil::exists(server_datadir)) {
    return ReturnCode::error(
        "EIO",
        "data dir not found: %s",
        server_datadir.c_str());
  }

  auto server_name = cfg_->getString("server.name");
  auto trash_dir = FileUtil::joinPaths(server_datadir, "trash");
  auto cache_dir = FileUtil::joinPaths(server_datadir, "cache");
  String tsdb_dir;
  String metadata_dir;
  if (server_name.isEmpty()) {
    tsdb_dir = FileUtil::joinPaths(server_datadir, "data/__anonymous");
    metadata_dir = FileUtil::joinPaths(server_datadir, "metadata/__anonymous");
  } else {
    tsdb_dir = FileUtil::joinPaths(
        server_datadir,
        "data/" + server_name.get());
    metadata_dir = FileUtil::joinPaths(
        server_datadir,
        "metadata/" + server_name.get());
  }

  try {
    if (!FileUtil::exists(tsdb_dir)) {
      FileUtil::mkdir_p(tsdb_dir);
    }

    if (!FileUtil::exists(metadata_dir)) {
      FileUtil::mkdir_p(metadata_dir);
    }

    if (!FileUtil::exists(trash_dir)) {
      FileUtil::mkdir(trash_dir);
    }

    if (!FileUtil::exists(cache_dir)) {
      FileUtil::mkdir(cache_dir);
    }
  } catch (const std::exception& e) {
    //shutdown();
    return ReturnCode::error("EIO", e.what());
  }

  try {
    server_lock_.reset(new FileLock(FileUtil::joinPaths(tsdb_dir, "__lock")));
    server_lock_->lock();
  } catch (const std::exception& e) {
    //shutdown();
    return ReturnCode::error("EIO", e.what());
  }

  /* config dir */
  {
    auto rc = ConfigDirectoryFactory::getConfigDirectoryForServer(
        cfg_,
        &config_dir_);

    if (rc.isSuccess()) {
      rc = config_dir_->start();
    }

    if (!rc.isSuccess()) {
      //shutdown();
      return ReturnCode::error(
          "ERUNTIME",
          "Can't connect to config backend: %s", rc.message().c_str());
    }
  }

  /* database context */
  database_context_.reset(new DatabaseContext());
  database_context_->db_path = tsdb_dir;
  database_context_->config = cfg_;
  database_context_->config_directory = config_dir_.get();

  /* file tracker */
  file_tracker_.reset(new FileTracker(trash_dir));

  /* garbage collector */
  try {
    auto gc_mode = garbageCollectorModeFromString(
        cfg_->getString("server.gc_mode").get());

    garbage_collector_.reset(
        new GarbageCollector(
            gc_mode,
            file_tracker_.get(),
            tsdb_dir,
            trash_dir,
            cache_dir,
            cfg_->getInt("server.cachedir_maxsize").get(),
            cfg_->getInt("server.gc_interval").get()));
  } catch (const std::exception& e) {
   // shutdown();
    return ReturnCode::error("ERUNTIME", e.what());
  }

  /* client auth */
  if (!cfg_->hasProperty("server.client_auth_backend")) {
    //shutdown();
    return ReturnCode::error("EARG", "missing 'server.client_auth_backend'");
  }

  auto client_auth_opt = cfg_->getString("server.client_auth_backend");
  if (client_auth_opt.get() == "trust") {
    client_auth_.reset(new TrustClientAuth());
  } else if (client_auth_opt.get() == "legacy") {
    if (!cfg_->hasProperty("server.legacy_auth_secret")) {
      //shutdown();
      return ReturnCode::error("EARG", "missing 'server.legacy_auth_secret'");
    }

    client_auth_.reset(
        new LegacyClientAuth(
            cfg_->getString("server.legacy_auth_secret").get()));
  } else {
    //shutdown();
    return ReturnCode::error(
        "EARG",
        "invalid client auth backend: " + client_auth_opt.get());
  }

  /* internal auth */
  internal_auth_.reset(new TrustInternalAuth());

  /* server allocator */
  server_alloc_.reset(new ServerAllocator(config_dir_.get(), cfg_));

  /* connection pool */
  dns_cache_.reset(new net::DNSCache());
  connection_pool_.reset(
      new native_transport::TCPConnectionPool(
          cfg_->getInt("server.s2s_pool_max_connections", 0),
          cfg_->getInt("server.s2s_pool_max_connections_per_host", 0),
          cfg_->getInt("server.s2s_pool_linger_timeout", 0),
          cfg_->getInt("server.s2s_io_timeout", 0)));

  /* metadata service */
  metadata_store_.reset(new MetadataStore(metadata_dir));
  metadata_cache_.reset(new MetadataCache());
  metadata_service_.reset(
      new MetadataService(
          database_context_.get(),
          metadata_store_.get(),
          metadata_cache_.get()));

  metadata_client_.reset(
      new MetadataClient(
          config_dir_.get(),
          cfg_,
          metadata_store_.get(),
          metadata_cache_.get(),
          connection_pool_.get(),
          dns_cache_.get()));

  metadata_coordinator_.reset(
      new MetadataCoordinator(
          config_dir_.get(),
          cfg_,
          connection_pool_.get(),
          dns_cache_.get()));

  /* server config */
  server_cfg_.reset(new ServerCfg());
  server_cfg_->db_path = tsdb_dir;
  server_cfg_->config_directory = config_dir_.get();
  server_cfg_->idx_cache = mkRef(new LSMTableIndexCache(tsdb_dir));
  server_cfg_->metadata_store = metadata_store_.get();
  server_cfg_->file_tracker = file_tracker_.get();

  /* core */
  partition_map_.reset(new PartitionMap(database_context_.get()));
  table_service_.reset(new TableService(database_context_.get()));

  replication_worker_.reset(
      new ReplicationWorker(
          partition_map_.get(),
          cfg_->getInt("server.replication_threads_max", 0)));

  compaction_worker_.reset(
      new CompactionWorker(
          partition_map_.get(),
          cfg_->getInt("server.indexbuild_threads").get()));

  /* metadata replication */
  if (!server_name.isEmpty()) {
    metadata_replication_.reset(
        new MetadataReplication(
            config_dir_.get(),
            cfg_,
            server_name.get(),
            metadata_store_.get(),
            connection_pool_.get(),
            dns_cache_.get()));
  }

  /* sql */
  {
    sql_query_cache_.reset(new csql::QueryCache(cache_dir));
    sql_symbols_ = mkRef(new csql::SymbolTable());
    csql::installDefaultSymbols(sql_symbols_.get());

    sql_ = mkRef(new csql::Runtime(
        thread::ThreadPoolOptions {
          .thread_name = Some(String("evqld-sql"))
        },
        sql_symbols_,
        new csql::QueryBuilder(
            new csql::ValueExpressionBuilder(sql_symbols_.get())),
        new csql::QueryPlanBuilder(
            csql::QueryPlanBuilderOptions{},
            sql_symbols_.get()),
        mkScoped(
            new Scheduler(
                cfg_,
                partition_map_.get(),
                config_dir_.get(),
                internal_auth_.get()))));

    sql_->setCacheDir(cache_dir);
    sql_->symbols()->registerFunction("version", &evqlVersionExpr);
    sql_->setQueryCache(sql_query_cache_.get());
  }

  /* spidermonkey javascript runtime */
  JS_Init();
  js::DisableExtraThreads();
  __eventql_mapreduce_prelude_js.registerAsset();

  /* more services */
  sql_service_.reset(
      new SQLService(
          sql_.get(),
          partition_map_.get(),
          config_dir_.get(),
          internal_auth_.get(),
          table_service_.get(),
          cache_dir));

  mapreduce_service_.reset(
      new MapReduceService(
          config_dir_.get(),
          internal_auth_.get(),
          table_service_.get(),
          partition_map_.get(),
          cache_dir));

  /* database context */
  {
    database_context_->partition_map = partition_map_.get();
    database_context_->file_tracker = file_tracker_.get();
    database_context_->replication_worker = replication_worker_.get();
    database_context_->lsm_index_cache = server_cfg_->idx_cache.get();
    database_context_->metadata_store = metadata_store_.get();
    database_context_->metadata_cache = metadata_cache_.get();
    database_context_->metadata_client = metadata_client_.get();
    database_context_->metadata_coordinator = metadata_coordinator_.get();
    database_context_->internal_auth = internal_auth_.get();
    database_context_->client_auth = client_auth_.get();
    database_context_->sql_runtime = sql_.get();
    database_context_->sql_service = sql_service_.get();
    database_context_->table_service = table_service_.get();
    database_context_->mapreduce_service = mapreduce_service_.get();
    database_context_->metadata_service = metadata_service_.get();
    database_context_->server_alloc = server_alloc_.get();
    database_context_->connection_pool = connection_pool_.get();
    database_context_->dns_cache = dns_cache_.get();
  }

  /* open tables */
  config_dir_->setTableConfigChangeCallback([this] (const TableDefinition& tbl) {
    Set<SHA1Hash> affected_partitions;
    try {
      partition_map_->configureTable(tbl, &affected_partitions);
    } catch (const std::exception& e) {
      logError(
          "evqld",
          "error while applying table config change to $0/$1",
          tbl.customer(),
          tbl.table_name());
    }

    for (const auto& partition_id : affected_partitions) {
      try {
        auto partition = partition_map_->findPartition(
            tbl.customer(),
            tbl.table_name(),
            partition_id);

        replication_worker_->enqueuePartition(
            partition.get(),
            (uint64_t) ReplicationWorker::ReplicationOptions::CORK);
      } catch (const std::exception& e) {
        logError(
            "evqld",
            "error while applying table config change to $0/$1/$2",
            tbl.customer(),
            tbl.table_name(),
            partition_id.toString());
      }
    }
  });

  config_dir_->listTables([this] (const TableDefinition& tbl) {
    partition_map_->configureTable(tbl);
  });

  /* start the database */
  replication_worker_->start();

  try {
    partition_map_->open();
  } catch (const std::exception& e) {
    //shutdown();
    return ReturnCode::error("ERUNTIME", e.what());
  }

  garbage_collector_->startGCThread();

  if (metadata_replication_.get()) {
    metadata_replication_->start();
  }

  if (!cfg_->getBool("server.noleader")) {
    leader_.reset(
        new Leader(
            config_dir_.get(),
            cfg_,
            server_alloc_.get(),
            metadata_client_.get(),
            cfg_->getInt("cluster.rebalance_interval").get(),
            connection_pool_.get(),
            dns_cache_.get()));

    leader_->startLeaderThread();
  }

  monitor_.reset(new Monitor(database_context_.get()));
  monitor_->startMonitorThread();

  return ReturnCode::success();
}

void DatabaseImpl::shutdown() {
  if (monitor_) {
    monitor_->stopMonitorThread();
  }

  if (leader_) {
    leader_->stopLeaderThread();
  }

  if (metadata_replication_) {
    metadata_replication_->stop();
  }

  if (replication_worker_) {
    replication_worker_->stop();
  }

  if (garbage_collector_) {
    garbage_collector_->stopGCThread();
  }

  if (config_dir_) {
    config_dir_->stop();
  }

  exit(0); // FIXME remove me once clean shutdown works
  monitor_.reset(nullptr);
  leader_.reset(nullptr);
  mapreduce_service_.reset(nullptr);
  sql_service_.reset(nullptr);
  sql_.reset(nullptr);
  sql_symbols_.reset(nullptr);
  sql_query_cache_.reset(nullptr);
  metadata_replication_.reset(nullptr);
  compaction_worker_.reset(nullptr);
  replication_worker_.reset(nullptr);
  table_service_.reset(nullptr);
  partition_map_.reset(nullptr);
  metadata_service_.reset(nullptr);
  metadata_store_.reset(nullptr);
  metadata_cache_.reset(nullptr);
  internal_auth_.reset(nullptr);
  client_auth_.reset(nullptr);
  server_alloc_.reset(nullptr);
  connection_pool_.reset(nullptr);
  dns_cache_.reset(nullptr);
  config_dir_.reset(nullptr);
  garbage_collector_.reset(nullptr);
  file_tracker_.reset(nullptr);
  database_context_.reset(nullptr);
  server_cfg_.reset(nullptr);
  server_lock_.reset(nullptr);
}

void DatabaseImpl::startThread(std::function<void(Session*)> entrypoint) {
  auto t = std::thread([this, entrypoint] () {
    auto session = new Session(database_context_.get());
    pthread_setspecific(local_session_, session);

    try {
      entrypoint(session);
    } catch (const std::exception& e) {
      logError("eventql", "Database Thread crashed: $0", e.what());
    }

    delete session;
  });

  t.detach();
}

Session* DatabaseImpl::getSession() const {
  return (Session*) pthread_getspecific(local_session_);
}

const ProcessConfig* DatabaseImpl::getConfig() const {
  return cfg_;
}

} // namespace tdsb

