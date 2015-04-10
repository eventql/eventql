/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_STRINGCOLUMNREADER_H
#define _FNORD_CSTABLE_STRINGCOLUMNREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/BitPackDecoder.h>
#include <fnord-cstable/ColumnReader.h>

namespace fnord {
namespace cstable {

class StringColumnReader : public ColumnReader {
public:

  StringColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size);

  bool next(
      uint64_t* rep_level,
      uint64_t* def_level,
      const char** data,
      size_t* data_len);

  bool next(
      uint64_t* rep_level,
      uint64_t* def_level,
      void** data,
      size_t* data_len) override;

protected:
  util::BinaryMessageReader data_reader_;
};

} // namespace cstable
} // namespace fnord

#endif
