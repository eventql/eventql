
/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRBySearchTermCrossCategoryMapper.h"

using namespace fnord;

namespace cm {

CTRBySearchTermCrossCategoryMapper::CTRBySearchTermCrossCategoryMapper(
    RefPtr<JoinedQueryTableSource> input,
    RefPtr<CTRCounterTableSink> output,
    const String& category_field,
    ItemEligibility eligibility,
    RefPtr<fts::Analyzer> analyzer,
    RefPtr<IndexReader> index) :
    Report(input.get(), output.get()),
    joined_queries_(input),
    ctr_table_(output),
    field_(category_field),
    eligibility_(eligibility),
    analyzer_(analyzer),
    index_(index) {}

void CTRBySearchTermCrossCategoryMapper::onInit() {
  joined_queries_->forEach(std::bind(
      &CTRBySearchTermCrossCategoryMapper::onJoinedQuery,
      this,
      std::placeholders::_1));
}

void CTRBySearchTermCrossCategoryMapper::onJoinedQuery(const JoinedQuery& q) {
  if (!isQueryEligible(eligibility_, q)) {
    return;
  }

  auto lang = extractLanguage(q.attrs);
  auto lang_str = languageToString(lang);

  auto qstr = extractAttr(q.attrs, "qstr~" + lang_str);
  if (qstr.isEmpty()) {
    return;
  }

  Vector<String> terms;
  try {
    analyzer_->normalize(lang, qstr.get(), &terms);
  } catch (const Exception& e) {
    fnord::logWarning(
        "cm.reportbuild",
        e,
        "error analyzing query: $0",
        qstr.get());

    return;
  }

  HashMap<String, CTRCounterData> per_field;
  for (auto& item : q.items) {
    if (!isItemEligible(eligibility_, q, item)) {
      continue;
    }

    Option<String> fval;
    try {
      fval = index_->docIndex()->getField(item.item.docID(), field_);
    } catch (const Exception& e) {
      fnord::logError("cm.reportbuild", e, "error");
    }

    if (fval.isEmpty()) {
      continue;
    }

    auto& c = per_field[fval.get()];
    ++c.num_views;
    if (item.clicked) ++c.num_clicks;
  }

  for (const auto& t : terms) {
    for (const auto& p : per_field) {
      auto key = StringUtil::format(
          "$0~$1~$2",
          lang_str,
          t,
          p.first);

      auto& c = counters_[key];
      c.num_views += p.second.num_views;
      c.num_clicks += p.second.num_clicks;
    }
  }
}

void CTRBySearchTermCrossCategoryMapper::onFinish() {
  for (auto& ctr : counters_) {
    ctr_table_->addRow(ctr.first, ctr.second);
  }
}


} // namespace cm

