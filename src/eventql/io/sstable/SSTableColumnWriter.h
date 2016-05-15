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
#ifndef _FNORD_SSTABLE_SSTABLECOLUMNWRITER_H
#define _FNORD_SSTABLE_SSTABLECOLUMNWRITER_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <eventql/util/buffer.h>
#include <eventql/util/exception.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/io/sstable/binaryformat.h>
#include <eventql/io/sstable/fileheaderreader.h>
#include <eventql/io/sstable/cursor.h>
#include <eventql/io/sstable/index.h>
#include <eventql/io/sstable/indexprovider.h>
#include <eventql/io/sstable/SSTableColumnSchema.h>


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


#endif
