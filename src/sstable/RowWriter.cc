/**
 * This file is part of the "libsstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <sstable/RowWriter.h>

namespace stx {
namespace sstable {

size_t RowWriter::appendRow(
    const FileHeader& hdr,
    void const* key,
    size_t key_size,
    void const* data,
    size_t data_size,
    OutputStream* os) {
  return 0;
}

}
}
