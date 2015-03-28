/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/TermInfoTableSource.h"
#include <fnord-sstable/SSTableColumnReader.h>
#include "fnord-json/json.h"

using namespace fnord;

namespace cm {

TermInfoTableSource::TermInfoTableSource(
    const Set<String>& sstable_filenames) :
    input_files_(sstable_filenames) {
  schema_.addColumn("related_terms", 1, sstable::SSTableColumnType::STRING);
}

void TermInfoTableSource::read() {
  int row_idx = 0;
  int tbl_idx = 0;
  for (const auto& sstable : input_files_) {
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    if (!reader.isFinalized()) {
      RAISEF(kRuntimeError, "unfinished sstable: $0", sstable);
    }

    if (reader.bodySize() == 0) {
      fnord::logWarning("cm.reportbuild", "Warning: empty sstable: $0", sstable);
    }

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      TermInfo ti;
      auto key = cursor->getKeyString();

      auto cols_data = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(&schema_, cols_data);
      auto terms_str = cols.getStringColumn(schema_.columnID("related_terms"));
      for (const auto& t : StringUtil::split(terms_str, " ")) {
        auto s = StringUtil::find(t, ':');
        if (s == String::npos) {
          continue;
        }

        ti.related_terms[t.substr(0, s)] += std::stoul(t.substr(s + 1));
      }

      for (const auto& cb : callbacks_) {
        cb(key, ti);
      }

      if (!cursor->next()) {
        break;
      }
    }
  }
}

void TermInfoTableSource::forEach(CallbackFn fn) {
  callbacks_.emplace_back(fn);
}

Set<String> TermInfoTableSource::inputFiles() {
  return input_files_;
}

} // namespace cm

