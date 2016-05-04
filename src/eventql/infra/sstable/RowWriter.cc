/**
 * This file is part of the "libsstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/fnv.h>
#include <eventql/infra/sstable/RowWriter.h>

namespace stx {
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
