/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_SSTABLE_SSTABLECOLUMNREADER_H
#define _FNORD_SSTABLE_SSTABLECOLUMNREADER_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <stx/buffer.h>
#include <stx/exception.h>
#include <stx/io/file.h>
#include <stx/io/mmappedfile.h>
#include <stx/util/binarymessagewriter.h>
#include <eventql/infra/sstable/binaryformat.h>
#include <eventql/infra/sstable/fileheaderreader.h>
#include <eventql/infra/sstable/cursor.h>
#include <eventql/infra/sstable/index.h>
#include <eventql/infra/sstable/indexprovider.h>
#include <eventql/infra/sstable/SSTableColumnSchema.h>

namespace stx {
namespace sstable {

class SSTableColumnReader {
public:

  SSTableColumnReader(SSTableColumnSchema* schema, const Buffer& buf);

  uint32_t getUInt32Column(SSTableColumnID id);
  uint64_t getUInt64Column(SSTableColumnID id);
  double getFloatColumn(SSTableColumnID id);

  String getStringColumn(SSTableColumnID id);
  Vector<String> getStringColumns(SSTableColumnID id);

protected:
  SSTableColumnSchema* schema_;
  Buffer buf_;
  util::BinaryMessageReader msg_reader_;
  Vector<std::tuple<SSTableColumnID, uint64_t, uint32_t>> col_data_;
};

} // namespace sstable
} // namespace stx

#endif
