/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_MESSAGESET_H
#define _FNORD_TSDB_MESSAGESET_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace tsdb {

class RecordSet {
public:

  RecordSet(
      RefPtr<msg::MessageSchema> schema,
      const String& filename_prefix);

  void addRecord(uint32_t record_id, const Buffer& message);

  void fetchRecord(uint32_t record_id, Buffer* message);

  Vector<uint32_t> listRecords();

protected:
  RefPtr<msg::MessageSchema> schema_;
  String filename_prefix_;
};

} // namespace tdsb
} // namespace fnord

#endif
