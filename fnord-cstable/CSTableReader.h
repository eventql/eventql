/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_CSTABLEREADER_H
#define _FNORD_CSTABLE_CSTABLEREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-cstable/ColumnWriter.h>

namespace fnord {
namespace cstable {

class CSTableReader {
public:

  CSTableReader(const String& filename);

  void getColumn(
      const String& column_name,
      void** data,
      size_t* size);

  size_t numRecords() const;

protected:
  io::MmappedFile file_;
  uint64_t num_records_;
  uint32_t num_columns_;

  struct ColumnInfo {
    String name;
    size_t body_offset;
    size_t size;
    uint64_t r_max;
    uint64_t d_max;
  };

  HashMap<String, ColumnInfo> columns_;
};

} // namespace cstable
} // namespace fnord

#endif
