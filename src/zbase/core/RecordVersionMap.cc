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
#include <stx/io/outputstream.h>
#include <stx/io/BufferedOutputStream.h>
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

} // namespace zbase
