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

void ZookeeperConfigDirectory::start() {
  zk_ = zookeeper_init(
      zookeeper_addrs_.c_str(),
      &zk_watch_cb,
      10000, /* recv timeout in microseconds */
      0, /* client id */
      this,
      0 /* flags */);

  if (!zk_) {
    RAISE_ERRNO("zookeeper_init failed");
  }
}

void ZookeeperConfigDirectory::stop() {
  zookeeper_close(zk_);
  zk_ = nullptr;
}

void ZookeeperConfigDirectory::handleZookeeperWatch(
    zhandle_t* zk,
    int type,
    int state,
    const char* path,
    void* ctx) {
  iputs("got zk watch... $0, $1, $2, $3", type, state, path, ctx);
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
