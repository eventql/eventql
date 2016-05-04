/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_SSTABLE_SSTABLECOLUMNWRITER_H
#define _FNORD_SSTABLE_SSTABLECOLUMNWRITER_H
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

class SSTableColumnWriter {
public:

  SSTableColumnWriter(SSTableColumnSchema* schema);

  void addUInt32Column(SSTableColumnID id, uint32_t value);
  void addUInt64Column(SSTableColumnID id, uint64_t value);
  void addFloatColumn(SSTableColumnID id, double value);
  void addStringColumn(SSTableColumnID id, const String& value);

  void* data() const;
  size_t size() const;

protected:
  SSTableColumnSchema* schema_;
  util::BinaryMessageWriter msg_writer_;
};

} // namespace sstable
} // namespace stx

#endif
