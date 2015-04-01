/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_UINT16COLUMNWRITER_H
#define _FNORD_CSTABLE_UINT16COLUMNWRITER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-cstable/ColumnWriter.h>

namespace fnord {
namespace cstable {

class UInt16ColumnWriter : public ColumnWriter {
public:

  UInt16ColumnWriter(uint64_t r_max, uint64_t d_max);

  void addDatum(uint64_t rep_level, uint64_t def_level, uint16_t value);

  void write(void** data, size_t* size);

protected:
  util::BinaryMessageWriter writer_;
};

} // namespace cstable
} // namespace fnord

#endif
