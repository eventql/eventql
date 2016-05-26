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
#include <eventql/config/config_directory_zookeeper.h>
#include <eventql/util/protobuf/msg.h>

namespace eventql {

static int free_String_vector(struct String_vector *v) {
  if (v->data) {
    int32_t i;
    for(i=0;i<v->count; i++) {
      free(v->data[i]);
    }
    free(v->data);
    v->data = 0;
  }
  return 0;
}

ZookeeperConfigDirectory::ZookeeperConfigDirectory(
    const String& cluster_name,
    const String& zookeeper_addrs) :
    cluster_name_(cluster_name),
    zookeeper_addrs_(zookeeper_addrs),
    zookeeper_timeout_(10000),
    path_prefix_(StringUtil::format("/eventql/$0", cluster_name_)),
    state_(ZKState::INIT),
    zk_(nullptr) {
  zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
}

ZookeeperConfigDirectory::~ZookeeperConfigDirectory() {
  if (zk_) {
    zookeeper_close(zk_);
  }
}

static void zk_watch_cb(
    zhandle_t* zk,
    int type,
    int state,
    const char* path,
    void* ctx) {
  auto self = (ZookeeperConfigDirectory*) zoo_get_context(zk);
  assert(self != nullptr);
  self->handleZookeeperWatch(zk, type, state, path, ctx);
}

Status ZookeeperConfigDirectory::start() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (state_ != ZKState::INIT) {
    return Status(eIllegalStateError, "state != ZK_INIT");
  }

  state_ = ZKState::CONNECTING;

  zk_ = zookeeper_init(
      zookeeper_addrs_.c_str(),
      &zk_watch_cb,
      zookeeper_timeout_,
      0, /* client id */
      this,
      0 /* flags */);

  if (!zk_) {
    return Status(eIOError, "zookeeper_init failed");
  }

  while (state_ < ZKState::LOADING) {
    logInfo("evqld", "Waiting for zookeeper ($0)", zookeeper_addrs_);
    cv_.wait_for(lk, std::chrono::seconds(1));
  }

  auto rc = sync();
  if (!rc.isSuccess()) {
    return rc;
  }

  state_ = ZKState::CONNECTED;
  return Status::success();
}

// PRECONDITION: must hold mutex
Status ZookeeperConfigDirectory::sync() {
  logInfo("evqld", "Loading config from zookeeper...");

  if (!getProtoNode(path_prefix_ + "/config", &cluster_config_, true)) {
    return Status(
        eNotFoundError,
        StringUtil::format("Cluster '$0' does not exist", cluster_name_));
  }

  Vector<String> namespaces;
  {
    auto rc = listChildren(path_prefix_ + "/namespaces", &namespaces, true);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  for (const auto& ns : namespaces) {
    auto rc = syncNamespace(ns);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return Status::success();
}

// PRECONDITION: must hold mutex
Status ZookeeperConfigDirectory::syncNamespace(const String& ns) {
  logDebug("evqld", "Loading namespace config from zookeeper: '$0'", ns);

  {
    auto key = StringUtil::format("$0/namespaces/$1/config", path_prefix_, ns);
    NamespaceConfig ns_config;
    if (!getProtoNode(key, &ns_config, true)) {
      return Status(
          eNotFoundError,
          StringUtil::format("namespace '$0' does not exist", ns));
    }

    namespaces_[ns] = ns_config;
  }

  Vector<String> tables;
  {
    auto key = StringUtil::format("$0/namespaces/$1/tables", path_prefix_, ns);
    auto rc = listChildren(key, &tables, true);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  for (const auto& table : tables) {
    auto rc = syncTable(ns, table);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return Status::success();
}

// PRECONDITION: must hold mutex
Status ZookeeperConfigDirectory::syncTable(
    const String& ns,
    const String& table_name) {
  logDebug(
      "evqld",
      "Loading table config from zookeeper: '$0/$1'",
      ns,
      table_name);

  auto key = StringUtil::format(
      "$0/namespaces/$1/tables/$2",
      path_prefix_,
      ns,
      table_name);

  TableDefinition tbl_config;
  if (!getProtoNode(key, &tbl_config, true)) {
    return Status(
        eNotFoundError,
        StringUtil::format("table '$0/$1' does not exist", ns, table_name));
  }

  tables_[ns + "~" + table_name] = tbl_config;
  return Status::success();
}

// PRECONDITION: must NOT hold mutex
void ZookeeperConfigDirectory::stop() {
  zookeeper_close(zk_);
  zk_ = nullptr;
}

bool ZookeeperConfigDirectory::getNode(
    String key,
    Buffer* buf,
    bool watch /* = false */,
    struct Stat* stat /* = nullptr */) {
  int buf_len = buf->size();

  auto rc = zoo_get(
      zk_,
      key.c_str(),
      0,
      (char*) buf->data(),
      &buf_len,
      stat);

  if (rc) {
    return false;
  }

  if (buf_len > 0) {
    buf->resize(buf_len);
  } else {
    buf->resize(0);
  }

  return true;
}

Status ZookeeperConfigDirectory::listChildren(
    String key,
    Vector<String>* children,
    bool watch /* = false */) {
  struct String_vector buf;
  buf.count = 0;
  buf.data = NULL;

  int rc = zoo_get_children(zk_, key.c_str(), watch, &buf);
  for (int i = 0; i < buf.count; ++i) {
    children->emplace_back(buf.data[i]);
  }
  free_String_vector(&buf);

  if (rc) {
    return Status(eIOError, getErrorString(rc));
  } else {
    return Status::success();
  }
}

/**
 * NOTE: if you're reading this and thinking  "wow, paul doesn't know what a
 * switch statement is, what an idiot" then think again. zookeeper.h defines
 * all states as extern int's so we can't switch over them :(
 */

void ZookeeperConfigDirectory::handleZookeeperWatch(
    zhandle_t* zk,
    int type,
    int state,
    const char* path,
    void* ctx) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (type == ZOO_SESSION_EVENT) {
    handleSessionEvent(zk, type, state, path, ctx);
  }

  cv_.notify_all();
}

void ZookeeperConfigDirectory::handleSessionEvent(
    zhandle_t* zk,
    int type,
    int state,
    const char* path,
    void* ctx) {

  if (state == ZOO_EXPIRED_SESSION_STATE ||
      state == ZOO_EXPIRED_SESSION_STATE ||
      state == ZOO_AUTH_FAILED_STATE) {
    handleConnectionLost();
    return;
  }

  if (state == ZOO_CONNECTED_STATE) {
    handleConnectionEstablished();
    return;
  }

  if (state == ZOO_CONNECTING_STATE ||
      state == ZOO_ASSOCIATING_STATE) {
    return;
  }

  logCritical("evqld", "ERROR: invalid zookeeper state: $0", state);
  handleConnectionLost();
}

void ZookeeperConfigDirectory::handleConnectionEstablished() {
  switch (state_) {
    case ZKState::CONNECTING:
      logInfo("evqld", "Zookeeper connection established");
      state_ = ZKState::LOADING;
      return;

    case ZKState::CONNECTION_LOST:
      logInfo("evqld", "Zookeeper connection re-established");
      return;

    case ZKState::INIT:
    case ZKState::LOADING:
    case ZKState::CLOSED:
    case ZKState::CONNECTED:
      logCritical(
          "evqld",
          "ERROR: invalid zookeeper state in handleConnectionEstablished");
      return;
  }
}

void ZookeeperConfigDirectory::handleConnectionLost() {
  switch (state_) {
    case ZKState::CONNECTING:
      logError("evqld", "Zookeeper connection failed");
      return;

    case ZKState::LOADING:
      logInfo("evqld", "Zookeeper connection lost while loading");
      state_ = ZKState::CONNECTING;
      return;

    case ZKState::CONNECTED:
    case ZKState::CONNECTION_LOST:
      logWarning("evqld", "Zookeeper connection lost");
      state_ = ZKState::CONNECTION_LOST;
      return;

    case ZKState::INIT:
    case ZKState::CLOSED:
      state_ = ZKState::CONNECTION_LOST;
      logCritical(
          "evqld",
          "ERROR: invalid zookeeper state in handleConnectionLost");
      return;
  }
}

ClusterConfig ZookeeperConfigDirectory::getClusterConfig() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return cluster_config_;
}

void ZookeeperConfigDirectory::updateClusterConfig(ClusterConfig config) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::setClusterConfigChangeCallback(
    Function<void (const ClusterConfig& cfg)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_cluster_change_.emplace_back(fn);
}

RefPtr<NamespaceConfigRef> ZookeeperConfigDirectory::getNamespaceConfig(
    const String& customer_key) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = namespaces_.find(customer_key);
  if (iter == namespaces_.end()) {
    RAISEF(kNotFoundError, "namespace not found: $0", customer_key);
  }

  return mkRef(new NamespaceConfigRef(iter->second));
}

void ZookeeperConfigDirectory::listNamespaces(
    Function<void (const NamespaceConfig& cfg)> fn) const {
  std::unique_lock<std::mutex> lk(mutex_);
  for (const auto& ns : namespaces_) {
    fn(ns.second);
  }
}

void ZookeeperConfigDirectory::setNamespaceConfigChangeCallback(
    Function<void (const NamespaceConfig& cfg)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_namespace_change_.emplace_back(fn);
}

void ZookeeperConfigDirectory::updateNamespaceConfig(NamespaceConfig cfg) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

TableDefinition ZookeeperConfigDirectory::getTableConfig(
    const String& db_namespace,
    const String& table_name) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = tables_.find(table_name);
  if (iter == tables_.end()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  return iter->second;
}

void ZookeeperConfigDirectory::createTableConfig(
    const TableDefinition& table) {
  auto path = StringUtil::format(
      "$0/namespaces/$1/tables/$2",
      path_prefix_,
      table.customer(),
      table.table_name());

  auto buf = msg::encode(table);
  auto rc = zoo_create(
      zk_,
      path.c_str(),
      (const char*) buf->data(),
      buf->size(),
      &ZOO_OPEN_ACL_UNSAFE,
      0,
      NULL /* path_buffer */,
      0 /* path_buffer_len */);

  if (rc) {
    RAISEF(kRuntimeError, "zoo_create() failed: $0", getErrorString(rc));
  }
}

void ZookeeperConfigDirectory::updateTableConfig(
    const TableDefinition& table,
    bool force /* = false */) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (table.version() == 0) {
    return createTableConfig(table);
  }

  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::listTables(
    Function<void (const TableDefinition& table)> fn) const {
  std::unique_lock<std::mutex> lk(mutex_);
  for (const auto& tbl : tables_) {
    fn(tbl.second);
  }
}

void ZookeeperConfigDirectory::setTableConfigChangeCallback(
    Function<void (const TableDefinition& tbl)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_table_change_.emplace_back(fn);
}

const char* ZookeeperConfigDirectory::getErrorString(int err) const {
  switch (err) {
    case ZOK: return "Everything is OK";
    case ZSYSTEMERROR: return "System Error";
    case ZRUNTIMEINCONSISTENCY: return "A runtime inconsistency was found";
    case ZDATAINCONSISTENCY: return "A data inconsistency was found";
    case ZCONNECTIONLOSS: return "Connection to the server has been lost";
    case ZMARSHALLINGERROR: return "Error while marshalling or unmarshalling data";
    case ZUNIMPLEMENTED: return "Operation is unimplemented";
    case ZOPERATIONTIMEOUT: return "Operation timeout";
    case ZBADARGUMENTS: return "Invalid arguments";
    case ZINVALIDSTATE: return "Invliad zhandle state";
    case ZAPIERROR: return "API Error";
    case ZNONODE: return "Node does not exist";
    case ZNOAUTH: return "Not authenticated";
    case ZBADVERSION: return "Version conflict";
    case ZNOCHILDRENFOREPHEMERALS: return "Ephemeral nodes may not have children";
    case ZNODEEXISTS: return "The node already exists";
    case ZNOTEMPTY: return "The node has children";
    case ZSESSIONEXPIRED: return "The session has been expired by the server";
    case ZINVALIDCALLBACK: return "Invalid callback specified";
    case ZINVALIDACL: return "Invalid ACL specified";
    case ZAUTHFAILED: return "Client authentication failed";
    case ZCLOSING: return "ZooKeeper is closing";
    case ZNOTHING: return "(not error) no server responses to process";
    case ZSESSIONMOVED: return "session moved to another server, so operation is ignored";
    default: return "ZooKeeper Error";
  }
}

} // namespace eventql
