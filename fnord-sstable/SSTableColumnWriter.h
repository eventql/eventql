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
#include <fnord-base/buffer.h>
#include <fnord-base/exception.h>
#include <fnord-base/io/file.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-sstable/binaryformat.h>
#include <fnord-sstable/fileheaderreader.h>
#include <fnord-sstable/cursor.h>
#include <fnord-sstable/index.h>
#include <fnord-sstable/indexprovider.h>

namespace fnord {
namespace sstable {

class SSTableColumnWriter {
public:

  SSTableColumnWriter(SSTableColumnSchema* schema);

  void addUINT32Column(SSTableColumnID id, uint32_t value);

protected:
  SSTableColumnSchema* schema_;
  util::BinaryMessageWriter msg_writer_;
};

} // namespace sstable
} // namespace fnord

#endif
