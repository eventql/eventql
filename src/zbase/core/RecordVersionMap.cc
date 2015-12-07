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
#include <stx/io/mmappedfile.h>
#include <stx/io/BufferedOutputStream.h>
#include <stx/logging.h>
#include <zbase/core/RecordVersionMap.h>
#include <cstable/CSTableWriter.h>

using namespace stx;

namespace zbase {

void RecordVersionMap::write(
    const OrderedMap<SHA1Hash, uint64_t>& map,
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
  io::MmappedFile mmap(File::openFile(filename, File::O_READ));

  auto len = *mmap.structAt<uint64_t>(1);
  if (len == 0) {
    return;
  }

  static size_t kHeaderOffset = 9;
  static size_t kSlotSize = 28;

  for (auto& p : *map) {
    uint64_t begin = 0;
    uint64_t end = len - 1;

    // continue searching while [begin,end] is not empty
    while (begin <= end) {
      uint64_t cur = begin + ((end - begin) / uint64_t(2));
      SHA1Hash cur_id(
          mmap.structAt<void>(kHeaderOffset + cur * kSlotSize),
          SHA1Hash::kSize);

      if (cur_id == p.first) {
        auto cur_version = *mmap.structAt<uint64_t>(
            kHeaderOffset + cur * kSlotSize + SHA1Hash::kSize);

        if (cur_version > p.second) {
          p.second = cur_version;
        }

        break;
      }

      if (len == 1) {
        break;
      }

      if (cur_id < p.first) {
        begin = cur + 1;
      } else {
        end = cur - 1;
      }
    }
  }
}

void RecordVersionMap::load(
    HashMap<SHA1Hash, uint64_t>* map,
    const String& filename) {
  auto is = FileInputStream::openFile(filename);
  is->readUInt8();
  auto len = is->readUInt64();
  for (size_t i = 0; i < len; ++i) {
    SHA1Hash id;
    is->readNextBytes(const_cast<void*>(id.data()), SHA1Hash::kSize);
    auto ver = is->readUInt64();
    auto& slot = (*map)[id];
    if (ver > slot) {
      slot = ver;
    }
  }
}

} // namespace zbase
