/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/search/RelatedTermsMapper.h"

using namespace stx;

namespace zbase {

RelatedTermsMapper::RelatedTermsMapper(
    RefPtr<AnalyticsTableScanSource> input,
    RefPtr<TermInfoTableSink> output) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_table_(output),
    time_col_(input->tableScan()->fetchColumn("search_queries.time")),
    qstr_col_(input->tableScan()->fetchColumn("search_queries.query_string_normalized")),
    lang_col_(input->tableScan()->fetchColumn("search_queries.language")) {}

void RelatedTermsMapper::onInit() {
  input_->tableScan()->onQuery(
      std::bind(&RelatedTermsMapper::onQuery, this));
}

void RelatedTermsMapper::onQuery() {
  auto terms = StringUtil::split(qstr_col_->getString(), " ");
  auto lang = languageToString((Language) lang_col_->getUInt32());

  auto& l_map = counters_[lang];
  for (int j = 0; j < terms.size(); ++j) {
    auto& t_info = l_map[terms[j]];
    t_info.score += 1;

    for (int i = 0; i < terms.size(); ++i) {
      if (i == j) {
        continue;
      }

      t_info.related_terms[terms[i]] += 1;
    }
  }
}

void RelatedTermsMapper::onFinish() {
  for (const auto& l : counters_) {
    for (const auto& t : l.second) {
      output_table_->addRow(l.first + "~" + t.first, t.second);
    }
  }
}

} // namespace zbase

