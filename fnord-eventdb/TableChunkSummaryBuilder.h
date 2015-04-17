/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLECHUNKSUMMARYBUILDER_H
#define _FNORD_EVENTDB_TABLECHUNKSUMMARYBUILDER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>

namespace fnord {
namespace eventdb {
class TableChunkSummaryWriter;

class TableChunkSummaryBuilder : public RefCounted {
public:
  virtual ~TableChunkSummaryBuilder() {}

  virtual void addRecord(const msg::MessageObject& record) = 0;
  virtual void commit(TableChunkSummaryWriter* writer) = 0;

};

}
}
#endif
