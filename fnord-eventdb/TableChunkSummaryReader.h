/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLECHUNKSUMMARYREADER_H
#define _FNORD_EVENTDB_TABLECHUNKSUMMARYREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/option.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>

namespace fnord {
namespace eventdb {

class TableChunkSummaryReader {
public:

  TableChunkSummaryReader(const String& filename);

  Option<Buffer> getSummary(const String& summary_name);

  bool getSummary(
      const String& summary_name,
      void** data,
      size_t* size);

protected:
  HashMap<String, Pair<uint64_t, uint64_t>> offsets_;
  io::MmappedFile mmap_;
  size_t body_offset_;
};

}
}
#endif
