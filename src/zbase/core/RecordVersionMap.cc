/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/stdtypes.h>
#include <stx/io/fileutil.h>
#include <stx/io/mmappedfile.h>
#include <stx/io/inputstream.h>
#include <stx/io/outputstream.h>
#include <stx/io/BufferedOutputStream.h>
#include <stx/logging.h>
#include <zbase/core/RecordVersionMap.h>

using namespace stx;

namespace zbase {

void RecordVersionMap::write(
    const HashMap<SHA1Hash, uint64_t>& map,
    const String& filename) {
  auto os = BufferedOutputStream::fromStream(
      FileOutputStream::openFile(filename));

  os->appendUInt8(0x1);
  os->appendUInt64(map.size());
  for (const auto& p : map) {
    os->write((const char*) p.first.data(),  p.first.size());
    os->appendUInt64(p.second);
  }
}

// FIXME !!!
void RecordVersionMap::lookup(
    HashMap<SHA1Hash, uint64_t>* map,
    const String& filename) {
  auto is = FileInputStream::openFile(filename);
  is->readUInt8();
  auto len = is->readUInt64();
  for (size_t i = 0; i < len; ++i) {
    SHA1Hash id;
    is->readNextBytes(const_cast<void*>(id.data()), SHA1Hash::kSize);
    auto ver = is->readUInt64();

    auto map_iter = map->find(id);
    if (map_iter != map->end() && map_iter->second <= ver) {
      map->erase(map_iter);
    }
  }
}

} // namespace zbase
