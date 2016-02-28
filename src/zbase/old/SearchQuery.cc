/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/util/fts.h"
#include "zbase/util/fts_common.h"
#include "zbase/util/search/DisjunctionMaxQuery.h"
#include "SearchQuery.h"

using namespace stx;

namespace zbase {

SearchQuery::SearchQuery() : total_hits_(0) {
  results_ = fts::TopScoreDocCollector::create(500, false);
}

void SearchQuery::addField(const String& field_name, double boost) {
  FieldInfo fi;
  fi.field_name = StringUtil::convertUTF8To16(field_name);
  fi.boost = boost;
  fields_.emplace_back(fi);
}

void SearchQuery::addTerm(const String& term) {
  terms_.emplace(term);
}

void SearchQuery::addQuery(
    const String& query,
    stx::Language lang,
    stx::fts::Analyzer* analyzer) {
  analyzer->extractTerms(lang, query, [this] (const String& t) {
    addTerm(t);
  });
}

void SearchQuery::execute(IndexReader* index) {
  auto query = fts::newLucene<fts::BooleanQuery>();

  for (const auto& t : terms_) {
    auto dm_query = fts::newLucene<fts::DisjunctionMaxQuery>();
    auto term = StringUtil::convertUTF8To16(t);

    for (const auto& f : fields_) {
      auto t_query = fts::newLucene<fts::TermQuery>(
          fts::newLucene<fts::Term>(f.field_name, term));

      t_query->setBoost(f.boost);
      dm_query->add(t_query);
    }

    query->add(dm_query, fts::BooleanClause::MUST);
  }

  auto searcher = index->ftsSearcher();
  searcher->search(query, results_);

  total_hits_ = results_->getTotalHits();
  results_->topDocs()->forEach([&searcher, this] (fts::ScoreDoc* sdoc) -> bool {
    auto doc = searcher->doc(sdoc->doc);
    res_docids_.emplace_back(StringUtil::convertUTF16To8(doc->get(L"_docid")));
    return true;
  });
}

void SearchQuery::writeJSON(json::JSONOutputStream* json) {
  json->beginObject();

  json->addObjectEntry("total_hits");
  json->addInteger(total_hits_);
  json->addComma();

  json->addObjectEntry("result_groups");
  json->beginArray();
  json->beginObject();
  json->addObjectEntry("group_title");
  json->addString("default");
  json->addComma();
  json->addObjectEntry("ids");
  json->beginArray();
  for (int i = 0; i < res_docids_.size(); ++i) {
    if (i > 0) {
      json->addComma();
    }

    json->addString(res_docids_[i]);
  }
  json->endArray();

  json->endObject();
  json->endArray();

  json->endObject();
}

}
