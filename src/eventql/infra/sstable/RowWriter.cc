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
#include <eventql/util/fnv.h>
#include <eventql/infra/sstable/RowWriter.h>

namespace util {
namespace sstable {

size_t RowWriter::appendRow(
    const MetaPage& hdr,
    void const* key,
    uint32_t key_size,
    void const* data,
    uint32_t data_size,
    OutputStream* os) {
  FNV<uint32_t> fnv;
  fnv.hash(&key_size, sizeof(key_size));
  fnv.hash(&data_size, sizeof(data_size));
  fnv.hash(key, key_size);
  fnv.hash(data, data_size);
  auto checksum = fnv.get();

  os->appendUInt32(checksum);
  os->appendUInt32(key_size);
  os->appendUInt32(data_size);

  os->write((char*) key, key_size);
  os->write((char*) data, data_size);

  return 12 + key_size + data_size;
}

}
}
