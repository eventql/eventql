/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_COLUMNWRITER_H
#define _FNORD_CSTABLE_COLUMNWRITER_H
#include <fnord-base/stdtypes.h>

namespace fnord {
namespace cstable {

class ColumnWriter {
public:
  virtual ~ColumnWriter() {}
  virtual void write(void** data, size_t* size) = 0;
};

} // namespace cstable
} // namespace fnord

#endif
