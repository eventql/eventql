/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <master/ConfigDirectoryMaster.h>
#include <stx/io/file.h>
#include <stx/io/fileutil.h>
#include <stx/random.h>
#include <stx/protobuf/msg.h>
#include <stx/logging.h>
#include <stx/inspect.h>

using namespace stx;

namespace cm {

ConfigDirectoryMaster::ConfigDirectoryMaster(
    const String& path) :
    db_path_(path) {
  loadHeads();
}

CustomerConfig ConfigDirectoryMaster::fetchCustomerConfig(
    const String& customer_key) const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto cpath = FileUtil::joinPaths(db_path_, customer_key);
  auto hpath = FileUtil::joinPaths(cpath, "config.HEAD");

  if (!FileUtil::exists(hpath)) {
    RAISEF(kNotFoundError, "customer not found: $0", customer_key);
  }

  auto head_version = std::stoull(FileUtil::read(hpath).toString());
  auto vpath = FileUtil::joinPaths(
      cpath,
      StringUtil::format("config.$0", head_version));

  return msg::decode<CustomerConfig>(FileUtil::read(vpath));
}

CustomerConfig ConfigDirectoryMaster::updateCustomerConfig(
    CustomerConfig config) {
  std::unique_lock<std::mutex> lk(mutex_);
  uint64_t head_version = 0;

  auto cpath = FileUtil::joinPaths(db_path_, config.customer());
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
  auto cpath = FileUtil::joinPaths(db_path_, customer_key);
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
  auto cpath = FileUtil::joinPaths(db_path_, customer_key);
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
  auto cpath = FileUtil::joinPaths(db_path_, "userdb");
  auto hpath = FileUtil::joinPaths(cpath, "users.HEAD");

  auto head_version = FileUtil::read(hpath).toString();

  auto users = msg::decode<UserDB>(
      FileUtil::read(
          FileUtil::joinPaths(
              cpath,
              StringUtil::format("users.$0", head_version))));

  users.set_version(std::stoull(head_version));
  return users;
}

UserConfig ConfigDirectoryMaster::updateUserConfig(
    UserConfig td,
    bool force /* = false */) {
  std::unique_lock<std::mutex> lk(mutex_);
  uint64_t head_version = 0;

  auto cpath = FileUtil::joinPaths(db_path_, "users");
  auto hpath = FileUtil::joinPaths(cpath, "users.HEAD");

  UserDB users;
  if (FileUtil::exists(cpath)) {
    if (FileUtil::exists(hpath)) {
      auto head_version_str = FileUtil::read(hpath);
      head_version = std::stoull(head_version_str.toString());
      users = msg::decode<UserDB>(
          FileUtil::read(
              FileUtil::joinPaths(
                  cpath,
                  StringUtil::format("users.$0", head_version))));
    }
  } else {
    FileUtil::mkdir(cpath);
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
      cpath,
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
  FileUtil::ls(db_path_, [this] (const String& customer) -> bool {
    if (customer == "userdb") {
      return true;;
    }

    {
      auto hpath = FileUtil::joinPaths(db_path_, customer + "/config.HEAD");
      if (FileUtil::exists(hpath)) {
        heads_["customers/" + customer] =
            std::stoull(FileUtil::read(hpath).toString());
      }
    }

    {
      auto hpath = FileUtil::joinPaths(db_path_, customer + "/tables.HEAD");
      if (FileUtil::exists(hpath)) {
        heads_["tables/" + customer] =
            std::stoull(FileUtil::read(hpath).toString());
      }
    }

    return true;
  });

  {
    auto hpath = FileUtil::joinPaths(db_path_, "userdb/users.HEAD");
    if (FileUtil::exists(hpath)) {
      heads_["userdb"] =
          std::stoull(FileUtil::read(hpath).toString());
    }
  }

}

} // namespace cm
