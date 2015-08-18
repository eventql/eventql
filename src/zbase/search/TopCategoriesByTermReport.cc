/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/search/TopCategoriesByTermReport.h"

using namespace stx;

namespace cm {

TopCategoriesByTermReport::TopCategoriesByTermReport(
    RefPtr<TermInfoTableSource> input,
    RefPtr<CSVSink> output) :
    ReportRDD(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

void TopCategoriesByTermReport::onInit() {
  input_table_->forEach(std::bind(
      &TopCategoriesByTermReport::onTermInfo,
      this,
      std::placeholders::_1,
      std::placeholders::_2));

  output_table_->addRow(Vector<String> {
    "language",
    "term",
    "category",
    "score"
  });
}

void TopCategoriesByTermReport::onTermInfo(const String& term, const TermInfo& ti) {
  for (const auto& rel : ti.top_categories) {
    Vector<String> row = StringUtil::split(term, "~");
    row.emplace_back(rel.first);
    row.emplace_back(StringUtil::toString(rel.second));
    output_table_->addRow(row);
  }
}

void TopCategoriesByTermReport::onFinish() {
}

} // namespace cm

