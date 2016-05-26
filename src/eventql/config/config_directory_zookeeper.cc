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

ZookeeperConfigDirectory::ZookeeperConfigDirectory(
    const String& cluster_name,
    const String& zookeeper_addrs) :
    cluster_name_(cluster_name),
    zookeeper_addrs_(zookeeper_addrs),
    zookeeper_timeout_(10000),
    zk_(nullptr) {
  zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
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

bool ZookeeperConfigDirectory::start() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (state_ != ZKState::INIT) {
    return false;
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
    logFatal("evqld", "zookeeper_init failed");
    return false;
  }

  while (state_ < ZKState::LOADING) {
    logInfo("evqld", "Waiting for zookeeper ($0)", zookeeper_addrs_);
    cv_.wait_for(lk, std::chrono::seconds(1));
  }

  logInfo("evqld", "Loading config from zookeeper...");

  auto cluster_config_buf = getNode(
      StringUtil::format("/eventql/$0/config", cluster_name_));

  if (cluster_config_buf.size() == 0) {
    logFatal("evqld", "Cluster '$0' does not exist", cluster_name_);
    return false;
  }

  return true;
}

void ZookeeperConfigDirectory::stop() {
  zookeeper_close(zk_);
  zk_ = nullptr;
}

Buffer ZookeeperConfigDirectory::getNode(
    String key,
    struct Stat* stat /* = nullptr */) {
  int buf_len = 4096;
  Buffer buf(buf_len);

  auto rc = zoo_get(
      zk_,
      key.c_str(),
      0,
      (char*) buf.data(),
      &buf_len,
      stat);

  if (rc) {
    RAISE(kRuntimeError, "zoo_get() failed: $0", rc);
  }

  if (buf_len > 0) {
    buf.resize(buf_len);
  } else {
    buf.resize(0);
  }

  return buf;
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
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::updateClusterConfig(ClusterConfig config) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::setClusterConfigChangeCallback(
    Function<void (const ClusterConfig& cfg)> fn) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

RefPtr<NamespaceConfigRef> ZookeeperConfigDirectory::getNamespaceConfig(
    const String& customer_key) const {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::listNamespaces(
    Function<void (const NamespaceConfig& cfg)> fn) const {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::setNamespaceConfigChangeCallback(
    Function<void (const NamespaceConfig& cfg)> fn) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::updateNamespaceConfig(NamespaceConfig cfg) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

TableDefinition ZookeeperConfigDirectory::getTableConfig(
    const String& db_namespace,
    const String& table_name) const {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::updateTableConfig(
    const TableDefinition& table,
    bool force /* = false */) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::listTables(
    Function<void (const TableDefinition& table)> fn) const {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

void ZookeeperConfigDirectory::setTableConfigChangeCallback(
    Function<void (const TableDefinition& tbl)> fn) {
  RAISE(kNotYetImplementedError, "zookeeper config directory not yet implemented");
}

} // namespace eventql
