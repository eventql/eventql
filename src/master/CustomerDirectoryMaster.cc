/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <master/CustomerDirectoryMaster.h>
#include <stx/io/file.h>
#include <stx/io/fileutil.h>
#include <stx/random.h>
#include <stx/protobuf/msg.h>
#include <stx/logging.h>
#include <stx/inspect.h>

using namespace stx;

namespace cm {

CustomerDirectoryMaster::CustomerDirectoryMaster(
    const String& path) :
    db_path_(path) {
  loadHeads();
}

void CustomerDirectoryMaster::updateCustomerConfig(CustomerConfig config) {
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

  auto config_buf = msg::encode(config);
  ++head_version;

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
}

void CustomerDirectoryMaster::loadHeads() {
  FileUtil::ls(db_path_, [this] (const String& customer) -> bool {
    auto hkey = customer + "/config";
    auto hpath = FileUtil::joinPaths(db_path_, hkey + ".HEAD");
    if (FileUtil::exists(hpath)) {
      heads_[hkey] = std::stoull(FileUtil::read(hpath).toString());
    }

    return true;
  });
}

} // namespace cm
