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
#include "eventql/util/assets.h"
#include "eventql/util/exception.h"
#include "eventql/util/io/fileutil.h"

namespace stx {

#ifndef _NDEBUG
static String __asset_search_path;
#endif

Assets::AssetMap* Assets::globalMap() {
  static AssetMap map;
  return &map;
}

Assets::AssetFile::AssetFile(
    const std::string& name,
    const unsigned char* data,
    size_t size) {
  Assets::registerAsset(name, data, size);
}

void Assets::registerAsset(
    const std::string& filename,
    const unsigned char* data,
    size_t size) {
  auto asset_map = Assets::globalMap();
  std::lock_guard<std::mutex> lock_holder(asset_map->mutex);
  asset_map->assets.emplace(filename, std::make_pair(data, size));
}

void Assets::setSearchPath(const String& search_path) {
#ifndef _NDEBUG
  __asset_search_path = search_path;
#endif
}

std::string Assets::getAsset(const std::string& filename) {
#ifndef _NDEBUG
  if (!__asset_search_path.empty()) {
    auto fpath = FileUtil::joinPaths(__asset_search_path, filename);
    if (FileUtil::exists(fpath)) {
      return FileUtil::read(fpath).toString();
    }
  }
#endif

  auto asset_map = Assets::globalMap();
  std::lock_guard<std::mutex> lock_holder(asset_map->mutex);
  const auto asset = asset_map->assets.find(filename);

  if (asset != asset_map->assets.end()) {
    const auto& data = asset->second;
    return std::string((const char*) data.first, data.second);
  }

  RAISE(kRuntimeError, "asset not found: %s", filename.c_str());
}

} // namespace stx

