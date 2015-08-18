/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/TermInfoTableSource.h"
#include <sstable/SSTableColumnReader.h>
#include "stx/json/json.h"

using namespace stx;

namespace cm {

TermInfoTableSource::TermInfoTableSource(
    const List<dproc::TaskDependency>& deps) :
    deps_(deps) {
  schema_.addColumn("related_terms", 1, sstable::SSTableColumnType::STRING);
  schema_.addColumn("top_categories", 2, sstable::SSTableColumnType::STRING);
  schema_.addColumn("score", 3, sstable::SSTableColumnType::FLOAT);
}

void TermInfoTableSource::read(dproc::TaskContext* context) {
  int row_idx = 0;
  int tbl_idx = 0;
  for (size_t i = 0; i < deps_.size(); ++i) {
    /* read sstable header */
    sstable::SSTableReader reader(context->getDependency(i)->getData());

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      TermInfo ti;
      auto key = cursor->getKeyString();

      auto cols_data = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(&schema_, cols_data);
      ti.score = cols.getFloatColumn(schema_.columnID("score"));

      auto terms_str = cols.getStringColumn(schema_.columnID("related_terms"));
      for (const auto& t : StringUtil::split(terms_str, ",")) {
        auto s = StringUtil::find(t, ':');
        if (s == String::npos) {
          continue;
        }

        ti.related_terms[t.substr(0, s)] += std::stoul(t.substr(s + 1));
      }

      auto topcats_str = cols.getStringColumn(
          schema_.columnID("top_categories"));
      for (const auto& c : StringUtil::split(topcats_str, ",")) {
        auto s = StringUtil::find(c, ':');
        if (s == String::npos) {
          continue;
        }

        ti.top_categories[c.substr(0, s)] += std::stod(c.substr(s + 1));
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

List<dproc::TaskDependency> TermInfoTableSource::dependencies() const {
  return deps_;
}

} // namespace cm

