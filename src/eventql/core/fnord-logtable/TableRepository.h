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
#ifndef _FNORD_LOGTABLE_TABLEREPOSITORY_H
#define _FNORD_LOGTABLE_TABLEREPOSITORY_H
#include <eventql/util/stdtypes.h>
#include <fnord-logtable/TableReader.h>
#include <fnord-logtable/TableWriter.h>
#include <fnord-afx/ArtifactIndex.h>
#include <eventql/util/protobuf/MessageSchema.h>

namespace stx {
namespace logtable {

class TableRepository {
public:

  TableRepository(
      const String& db_path,
      const String& replica_id,
      bool readonly,
      TaskScheduler* scheduler);

  void addTable(const String& table_name, const msg::MessageSchema& schema);

  RefPtr<TableSnapshot> getSnapshot(const String& table_name) const;

  RefPtr<TableWriter> findTableWriter(const String& table_name) const;
  RefPtr<TableReader> findTableReader(const String& table_name) const;
  Set<String> tables() const;

  const String& replicaID() const;

protected:
  String db_path_;
  String replica_id_;
  bool readonly_;
  HashMap<String, RefPtr<TableWriter>> table_writers_;
  HashMap<String, RefPtr<TableReader>> table_readers_;
  mutable std::mutex mutex_;
  TaskScheduler* scheduler_;
};

} // namespace logtable
} // namespace stx

#endif
