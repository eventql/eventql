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
#include "eventql/db/metadata_store.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/random.h"
#include "eventql/util/logging.h"

namespace eventql {

MetadataStore::MetadataStore(
    const String& path_prefix) :
    path_prefix_(path_prefix) {}

Status MetadataStore::getMetadataFile(
    const String& ns,
    const String& table_name,
    const SHA1Hash& txid,
    RefPtr<MetadataFile>* file) const {
  auto file_path = getPath(ns, table_name, txid);
  if (!FileUtil::exists(file_path)) {
    return Status(eIOError, "metadata file does not exist");
  }

  auto is = FileInputStream::openFile(file_path);
  file->reset(new MetadataFile());

  auto rc = file->get()->decode(is.get());
  if (!rc.isSuccess()) {
    return rc;
  }

  return Status::success();
}

bool MetadataStore::hasMetadataFile(
    const String& ns,
    const String& table_name,
    const SHA1Hash& txid) {
  auto file_path = getPath(ns, table_name, txid);
  return FileUtil::exists(file_path);
}

Status MetadataStore::storeMetadataFile(
    const String& ns,
    const String& table_name,
    const MetadataFile& file) {
  auto txid = file.getTransactionID();

  logDebug(
      "evqld",
      "Storing metadata file: $0/$1/$2",
      ns,
      table_name,
      txid.toString());

  auto file_path = getPath(ns, table_name, txid);
  auto file_path_tmp = file_path + "~" + Random::singleton()->hex64();

  FileUtil::mkdir_p(getBasePath(ns, table_name));
  auto os = FileOutputStream::openFile(file_path_tmp);

  auto rc = file.encode(os.get());
  if (!rc.isSuccess()) {
    return rc;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  if (FileUtil::exists(file_path)) {
    FileUtil::rm(file_path_tmp);
    return Status(eIOError, "metadata file already exists");
  } else {
    FileUtil::mv(file_path_tmp, file_path);
    return Status::success();
  }
}

String MetadataStore::getBasePath(
    const String& ns,
    const String& table_name) const {
  return FileUtil::joinPaths(
      path_prefix_,
      StringUtil::format("$0/$1", ns, table_name));
}

String MetadataStore::getPath(
    const String& ns,
    const String& table_name,
    const SHA1Hash& txid) const {
  return FileUtil::joinPaths(getBasePath(ns, table_name), txid.toString());
}

} // namespace eventql

