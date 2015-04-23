/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DOCTABLE_TABLESEGMENTWRITER_H
#define _FNORD_DOCTABLE_TABLESEGMENTWRITER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-base/thread/taskscheduler.h>
#include <fnord-logtable/ArtifactIndex.h>
#include <fnord-doctable/TableArena.h>
#include <fnord-doctable/TableSnapshot.h>
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"

namespace fnord {
namespace doctable {

class TableSegmentWriter : public RefCounted {
public:

  TableSegmentWriter(
      const String& db_path,
      const String& table_name,
      msg::MessageSchema* schema,
      TableSegmentRef* chunk);

  void addRecord(const msg::MessageObject& record);
  void addSummary(RefPtr<TableSegmentSummaryBuilder> summary);
  void commit();

protected:
  msg::MessageSchema* schema_;
  TableSegmentRef* chunk_;
  size_t seq_;
  String chunk_name_;
  String chunk_filename_;
  std::unique_ptr<sstable::SSTableWriter> sstable_;
  std::unique_ptr<cstable::CSTableBuilder> cstable_;
  List<RefPtr<TableSegmentSummaryBuilder>> summaries_;
};

} // namespace doctable
} // namespace fnord

#endif
