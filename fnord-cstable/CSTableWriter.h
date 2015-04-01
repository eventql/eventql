/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_CSTABLEWRITER_H
#define _FNORD_CSTABLE_CSTABLEWRITER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-cstable/ColumnWriter.h>

namespace fnord {
namespace cstable {

class CSTableWriter {
public:

  CSTableWriter(
      const String& filename,
      const uint64_t num_records);

  void addColumn(
      const String& column_name,
      ColumnWriter* column_writer);

  void commit();

protected:
  File file_;
  size_t offset_;

  struct ColumnInfo {
    String name;
    size_t body_offset;
    size_t size;
    ColumnWriter* writer;
  };

  List<ColumnInfo> columns_;
};

} // namespace cstable
} // namespace fnord

#endif
