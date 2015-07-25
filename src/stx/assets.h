/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_WEB_ASSETS_H
#define _libstx_WEB_ASSETS_H
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace stx {

class Assets {
public:
  class AssetFile {
  public:
    AssetFile(
        const std::string& name,
        const unsigned char* data,
        size_t size);
  };

  static void registerAsset(
      const std::string& filename,
      const unsigned char* data,
      size_t size);

  static std::string getAsset(const std::string& filename);

protected:

  struct AssetMap {
    std::mutex mutex;
    std::unordered_map<
        std::string,
        std::pair<const unsigned char*, size_t>> assets;
  };

  static AssetMap* globalMap();

};

} // namespace stx

#endif
