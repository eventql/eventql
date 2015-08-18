/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/search/RelatedTermsReport.h"

using namespace stx;

namespace cm {

RelatedTermsReport::RelatedTermsReport(
    RefPtr<TermInfoTableSource> input,
    RefPtr<CSVSink> output) :
    ReportRDD(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

void RelatedTermsReport::onInit() {
  input_table_->forEach(std::bind(
      &RelatedTermsReport::onTermInfo,
      this,
      std::placeholders::_1,
      std::placeholders::_2));

  output_table_->addRow(Vector<String> {
    "language",
    "term",
    "related_term",
    "score"
  });
}

void RelatedTermsReport::onTermInfo(const String& term, const TermInfo& ti) {
  for (const auto& rel : ti.related_terms) {
    if (rel.second < 10) {
      continue;
    }

    Vector<String> row = StringUtil::split(term, "~");
    row.emplace_back(rel.first);
    row.emplace_back(StringUtil::toString(rel.second));
    output_table_->addRow(row);
  }
}

void RelatedTermsReport::onFinish() {
}

} // namespace cm

