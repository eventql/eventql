/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _FNORD_LOGTABLE_TABLEREADER_H
#define _FNORD_LOGTABLE_TABLEREADER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <fnord-logtable/AbstractTableReader.h>
#include "eventql/io/sstable/sstablereader.h"
#include "eventql/io/cstable/CSTableReader.h"

namespace util {
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
} // namespace util

#endif
