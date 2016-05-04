/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLEARENA_H
#define _FNORD_LOGTABLE_TABLEARENA_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/MessageObject.h>

namespace stx {
namespace logtable {

class TableArena : public RefCounted {
public:

  TableArena(uint64_t start_sequence, const String& chunkid);

  void addRecord(const msg::MessageObject& record);

  const List<msg::MessageObject>& records() const;

  size_t startSequence() const;
  const String& chunkID() const;
  size_t size() const;

  bool isCommmited() const;
  void commit();

protected:
  size_t start_sequence_;
  String chunkid_;
  List<msg::MessageObject> records_;
  std::atomic<size_t> size_;
  std::atomic<size_t> is_committed_;
};

} // namespace logtable
} // namespace stx

#endif
