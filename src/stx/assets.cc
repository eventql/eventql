/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/assets.h"
#include "stx/exception.h"
#include "stx/io/fileutil.h"

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

