/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_ABSTRACTTABLEREADER_H
#define _FNORD_LOGTABLE_ABSTRACTTABLEREADER_H
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/protobuf/MessageSchema.h>
#include <stx/protobuf/MessageObject.h>
#include <fnord-logtable/TableArena.h>
#include <fnord-logtable/TableSnapshot.h>

namespace stx {
namespace logtable {

class AbstractTableReader : public RefCounted {
public:

  virtual ~AbstractTableReader() {}

  virtual const String& name() const = 0;
  virtual const msg::MessageSchema& schema() const = 0;

  virtual RefPtr<TableSnapshot> getSnapshot() = 0;

  virtual size_t fetchRecords(
      const String& replica,
      uint64_t start_sequence,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn) = 0;

};

} // namespace logtable
} // namespace stx

#endif
