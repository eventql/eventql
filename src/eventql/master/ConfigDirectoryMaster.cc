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
#include <eventql/master/ConfigDirectoryMaster.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/random.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/logging.h>
#include <eventql/util/inspect.h>

#include "eventql/eventql.h"

namespace eventql {

ConfigDirectoryMaster::ConfigDirectoryMaster(
    const String& path) :
    customerdb_path_(FileUtil::joinPaths(path, "customers")),
    userdb_path_(FileUtil::joinPaths(path, "userdb")),
    clusterdb_path_(FileUtil::joinPaths(path, "cluster")) {
  FileUtil::mkdir_p(customerdb_path_);
  FileUtil::mkdir_p(userdb_path_);
  loadHeads();
}

ClusterConfig ConfigDirectoryMaster::fetchClusterConfig() const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto hpath = FileUtil::joinPaths(clusterdb_path_, "cluster.HEAD");
  if (!FileUtil::exists(hpath)) {
    return ClusterConfig();
  }

  auto head_version = std::stoull(FileUtil::read(hpath).toString());
  auto vpath = FileUtil::joinPaths(
      clusterdb_path_,
      StringUtil::format("cluster.$0", head_version));

  return msg::decode<ClusterConfig>(FileUtil::read(vpath));
}

ClusterConfig ConfigDirectoryMaster::updateClusterConfig(ClusterConfig config) {
  std::unique_lock<std::mutex> lk(mutex_);
  uint64_t head_version = 0;

  auto hpath = FileUtil::joinPaths(clusterdb_path_, "cluster.HEAD");
  if (FileUtil::exists(hpath)) {
    auto head_version_str = FileUtil::read(hpath);
    head_version = std::stoull(head_version_str.toString());
  }

  if (config.version() != head_version) {
    RAISE(
        kRuntimeError,
        "VERSION MISMATCH: can't update customer config because the update is" \
        " out of date (i.e. it is not based on the latest head version)");
  }

  config.set_version(++head_version);
  auto config_buf = msg::encode(config);

  logInfo(
      "dxa-master",
      "Updating cluster config; head=$0",
      head_version);

  auto vpath = FileUtil::joinPaths(
      clusterdb_path_,
      StringUtil::format("cluster.$0", head_version));

  auto vtmppath = vpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(vtmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(config_buf->data(), config_buf->size());
  }

  auto htmppath = hpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(htmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(StringUtil::toString(head_version));
  }

  FileUtil::mv(vtmppath, vpath);
  FileUtil::mv(htmppath, hpath);
  heads_["cluster"] = head_version;

  return config;
}

NamespaceConfig ConfigDirectoryMaster::fetchNamespaceConfig(
    const String& customer_key) const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto cpath = FileUtil::joinPaths(customerdb_path_, customer_key);
  auto hpath = FileUtil::joinPaths(cpath, "config.HEAD");

  if (!FileUtil::exists(hpath)) {
    RAISEF(kNotFoundError, "customer not found: $0", customer_key);
  }

  auto head_version = std::stoull(FileUtil::read(hpath).toString());
  auto vpath = FileUtil::joinPaths(
      cpath,
      StringUtil::format("config.$0", head_version));

  return msg::decode<NamespaceConfig>(FileUtil::read(vpath));
}

NamespaceConfig ConfigDirectoryMaster::updateNamespaceConfig(
    NamespaceConfig config) {
  std::unique_lock<std::mutex> lk(mutex_);
  uint64_t head_version = 0;

  auto cpath = FileUtil::joinPaths(customerdb_path_, config.customer());
  auto hpath = FileUtil::joinPaths(cpath, "config.HEAD");
  if (FileUtil::exists(cpath)) {
    if (FileUtil::exists(hpath)) {
      auto head_version_str = FileUtil::read(hpath);
      head_version = std::stoull(head_version_str.toString());
    }
  } else {
    FileUtil::mkdir(cpath);
  }

  if (config.version() != head_version) {
    RAISE(
        kRuntimeError,
        "VERSION MISMATCH: can't update customer config because the update is" \
        " out of date (i.e. it is not based on the latest head version)");
  }

  config.set_version(++head_version);
  auto config_buf = msg::encode(config);

  logInfo(
      "dxa-master",
      "Updating customer config; customer=$0 head=$1",
      config.customer(),
      head_version);

  auto vpath = FileUtil::joinPaths(
      cpath,
      StringUtil::format("config.$0", head_version));

  auto vtmppath = vpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(vtmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(config_buf->data(), config_buf->size());
  }

  auto htmppath = hpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(htmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(StringUtil::toString(head_version));
  }

  FileUtil::mv(vtmppath, vpath);
  FileUtil::mv(htmppath, hpath);
  heads_["customers/" + config.customer()] = head_version;

  return config;
}

TableDefinition ConfigDirectoryMaster::fetchTableDefinition(
    const String& customer_key,
    const String& table_name) {
  auto tables = fetchTableDefinitions(customer_key);
  for (auto& tbl : tables.tables()) {
    if (tbl.table_name() == table_name) {
      return tbl;
    }
  }

  RAISEF(kNotFoundError, "table not found: $0", table_name);
}

TableDefinitionList ConfigDirectoryMaster::fetchTableDefinitions(
    const String& customer_key) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto cpath = FileUtil::joinPaths(customerdb_path_, customer_key);
  auto hpath = FileUtil::joinPaths(cpath, "tables.HEAD");

  if (!FileUtil::exists(cpath) || !FileUtil::exists(hpath)) {
    RAISEF(kNotFoundError, "customer not found: $0", customer_key);
  }

  auto head_version = FileUtil::read(hpath).toString();

  auto tables = msg::decode<TableDefinitionList>(
      FileUtil::read(
          FileUtil::joinPaths(
              cpath,
              StringUtil::format("tables.$0", head_version))));

  tables.set_version(std::stoull(head_version));
  return tables;
}

TableDefinition ConfigDirectoryMaster::updateTableDefinition(
    TableDefinition td,
    bool force /* = false */) {
  std::unique_lock<std::mutex> lk(mutex_);
  uint64_t head_version = 0;

  auto customer_key = td.customer();
  auto cpath = FileUtil::joinPaths(customerdb_path_, customer_key);
  auto hpath = FileUtil::joinPaths(cpath, "tables.HEAD");

  TableDefinitionList tables;
  if (FileUtil::exists(cpath)) {
    if (FileUtil::exists(hpath)) {
      auto head_version_str = FileUtil::read(hpath);
      head_version = std::stoull(head_version_str.toString());
      tables = msg::decode<TableDefinitionList>(
          FileUtil::read(
              FileUtil::joinPaths(
                  cpath,
                  StringUtil::format("tables.$0", head_version))));
    }
  } else {
    FileUtil::mkdir(cpath);
  }

  TableDefinition* head_td = nullptr;
  for (auto& tbl : *tables.mutable_tables()) {
    if (tbl.table_name() == td.table_name()) {
      head_td = &tbl;
    }
  }

  if (head_td == nullptr) {
    head_td = tables.add_tables();
  }

  if (force) {
    td.set_version(head_td->version());
  }

  if (td.version() != head_td->version()) {
    RAISE(
        kRuntimeError,
        "VERSION MISMATCH: can't update table definition because the update is" \
        " out of date (i.e. it is not based on the latest head version)");
  }

  *head_td = td;
  head_td->set_version(td.version() + 1);
  ++head_version;

  logInfo(
      "dxa-master",
      "Updating table config; customer=$0 table=$1 head=$2",
      customer_key,
      head_td->table_name(),
      head_td->version());

  auto td_buf = msg::encode(tables);

  auto vpath = FileUtil::joinPaths(
      cpath,
      StringUtil::format("tables.$0", head_version));

  auto vtmppath = vpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(vtmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(td_buf->data(), td_buf->size());
  }

  auto htmppath = hpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(htmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(StringUtil::toString(head_version));
  }

  FileUtil::mv(vtmppath, vpath);
  FileUtil::mv(htmppath, hpath);
  heads_["tables/" + customer_key] = head_version;

  return *head_td;
}

UserConfig ConfigDirectoryMaster::fetchUserConfig(
    const String& user_name) {
  auto users = fetchUserDB();
  for (auto& usr : users.users()) {
    if (usr.userid() == user_name) {
      return usr;
    }
  }

  RAISEF(kNotFoundError, "user not found: $0", user_name);
}

UserDB ConfigDirectoryMaster::fetchUserDB() {
  std::unique_lock<std::mutex> lk(mutex_);
  auto hpath = FileUtil::joinPaths(userdb_path_, "users.HEAD");

  auto head_version = FileUtil::read(hpath).toString();

  auto users = msg::decode<UserDB>(
      FileUtil::read(
          FileUtil::joinPaths(
              userdb_path_,
              StringUtil::format("users.$0", head_version))));

  users.set_version(std::stoull(head_version));
  return users;
}

UserConfig ConfigDirectoryMaster::updateUserConfig(
    UserConfig td,
    bool force /* = false */) {
  std::unique_lock<std::mutex> lk(mutex_);
  uint64_t head_version = 0;

  auto hpath = FileUtil::joinPaths(userdb_path_, "users.HEAD");

  UserDB users;
  if (FileUtil::exists(hpath)) {
    auto head_version_str = FileUtil::read(hpath);
    head_version = std::stoull(head_version_str.toString());
    users = msg::decode<UserDB>(
        FileUtil::read(
            FileUtil::joinPaths(
                userdb_path_,
                StringUtil::format("users.$0", head_version))));
  }

  UserConfig* head_cfg = nullptr;
  for (auto& usr : *users.mutable_users()) {
    if (usr.userid() == td.userid()) {
      head_cfg = &usr;
    }
  }

  if (head_cfg == nullptr) {
    head_cfg = users.add_users();
  }

  if (force) {
    td.set_version(head_cfg->version());
  }

  if (td.version() != head_cfg->version()) {
    RAISE(
        kRuntimeError,
        "VERSION MISMATCH: can't update user definition because the update is" \
        " out of date (i.e. it is not based on the latest head version)");
  }

  *head_cfg = td;
  head_cfg->set_version(td.version() + 1);
  ++head_version;

  logInfo(
      "dxa-master",
      "Updating user config; user=$1 head=$2",
      head_cfg->userid(),
      head_cfg->version());

  auto td_buf = msg::encode(users);

  auto vpath = FileUtil::joinPaths(
      userdb_path_,
      StringUtil::format("users.$0", head_version));

  auto vtmppath = vpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(vtmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(td_buf->data(), td_buf->size());
  }

  auto htmppath = hpath + "~tmp." + Random::singleton()->hex64();
  {
    auto tmpfile = File::openFile(htmppath, File::O_CREATE | File::O_WRITE);
    tmpfile.write(StringUtil::toString(head_version));
  }

  FileUtil::mv(vtmppath, vpath);
  FileUtil::mv(htmppath, hpath);
  heads_["userdb"] = head_version;

  return *head_cfg;
}

Vector<Pair<String, uint64_t>> ConfigDirectoryMaster::heads() const {
  std::unique_lock<std::mutex> lk(mutex_);
  Vector<Pair<String, uint64_t>> heads;

  for (const auto& h : heads_) {
    heads.emplace_back(h);
  }

  return heads;
}

void ConfigDirectoryMaster::loadHeads() {
  FileUtil::ls(customerdb_path_, [this] (const String& customer) -> bool {
    {
      auto hpath = FileUtil::joinPaths(customerdb_path_, customer + "/config.HEAD");
      if (FileUtil::exists(hpath)) {
        heads_["customers/" + customer] =
            std::stoull(FileUtil::read(hpath).toString());
      }
    }

    {
      auto hpath = FileUtil::joinPaths(customerdb_path_, customer + "/tables.HEAD");
      if (FileUtil::exists(hpath)) {
        heads_["tables/" + customer] =
            std::stoull(FileUtil::read(hpath).toString());
      }
    }

    return true;
  });

  {
    auto hpath = FileUtil::joinPaths(userdb_path_, "users.HEAD");
    if (FileUtil::exists(hpath)) {
      heads_["userdb"] =
          std::stoull(FileUtil::read(hpath).toString());
    }
  }

  {
    auto hpath = FileUtil::joinPaths(clusterdb_path_, "cluster.HEAD");
    if (FileUtil::exists(hpath)) {
      heads_["cluster"] =
          std::stoull(FileUtil::read(hpath).toString());
    }
  }

}

} // namespace eventql
