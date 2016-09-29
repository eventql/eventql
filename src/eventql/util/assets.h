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
#pragma once
#include <eventql/util/stdtypes.h>

class Assets {
public:
  class AssetFile {
  public:
    AssetFile(
        const std::string& name,
        const unsigned char* data,
        size_t size);

    void registerAsset();

  protected:
    String name_;
    const unsigned char* data_;
    size_t size_;
  };

  static void registerAsset(
      const std::string& filename,
      const unsigned char* data,
      size_t size);

  static std::string getAsset(const std::string& filename);

  /**
   * Not thread safe, debug build only
   */
  static void setSearchPath(const String& search_path);

protected:

  struct AssetMap {
    std::mutex mutex;
    std::unordered_map<
        std::string,
        std::pair<const unsigned char*, size_t>> assets;
  };

  static AssetMap* globalMap();
};

