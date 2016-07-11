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
#include "eventql/util/stdtypes.h"
#include "eventql/util/application.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/io/file.h"
#include "eventql/util/inspect.h"
#include "eventql/util/human.h"
#include "eventql/io/cstable/cstable_reader.h"
#include "eventql/eventql.h"

int main(int argc, const char** argv) {
  Application::init();

  String filename(argv[1]);
  auto cstable = cstable::CSTableReader::openFile(filename);
  iputs("== GENERAL ==\n >> number of records: $0", cstable->numRecords());

  iputs("\n\n== INDEX ==", 1);
  for (const auto& c : cstable->columns()) {
    iputs(
        ">>  column_id=$0, column_name=$1",
        c.column_id,
        c.column_name);
  }

  auto page_mgr = cstable->getPageManager();
  for (const auto& e : page_mgr->getPageIndex()) {
    String type_str;
    switch (e.key.entry_type) {
      case cstable::PageIndexEntryType::DLEVEL:
        type_str = "DLVL";
        break;
      case cstable::PageIndexEntryType::RLEVEL:
        type_str = "RLVL";
        break;
      case cstable::PageIndexEntryType::DATA:
        type_str = "DATA";
        break;
    }

    iputs(
        ">>  column_id=$0 type=$1 offset=$2 size=$3",
        e.key.column_id,
        type_str,
        e.page.offset,
        e.page.size);
  }

  for (const auto& c : cstable->columns()) {
    iputs(
        "\n\n== COLUMN DATA for $0/$1 ==",
        c.column_id,
        c.column_name);

    auto col_reader = cstable->getColumnReader(c.column_name);
    uint64_t rlevel;
    uint64_t dlevel;
    String data;
    for (auto i = cstable->numRecords(); i; ) {
      col_reader->readString(&rlevel, &dlevel, &data);
      iputs(">>  rlvl=$0 dlvl=$1 data=($2) '$3'", rlevel, dlevel, data.size(), data);

      if (col_reader->nextRepetitionLevel() == 0) {
        --i;
      }
    }
  }

  return 0;
}

