/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLEREADER_H
#define _FNORD_LOGTABLE_TABLEREADER_H
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <fnord-logtable/AbstractTableReader.h>
#include "eventql/infra/sstable/sstablereader.h"
#include "cstable/CSTableReader.h"

namespace stx {
namespace logtable {

class TableReader : public AbstractTableReader {
public:

  static RefPtr<TableReader> open(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema);

  const String& name() const override;
  const String& basePath() const;
  const msg::MessageSchema& schema() const override;

  RefPtr<TableSnapshot> getSnapshot() override;

  size_t fetchRecords(
      const String& replica,
      uint64_t start_sequence,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn) override;

protected:

  TableReader(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema,
      uint64_t head_generation);

  size_t fetchRecords(
      const TableChunkRef& chunk,
      size_t offset,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn);

  String name_;
  String replica_id_;
  String db_path_;
  msg::MessageSchema schema_;
  std::mutex mutex_;
  uint64_t head_gen_;
};

} // namespace logtable
} // namespace stx

#endif
