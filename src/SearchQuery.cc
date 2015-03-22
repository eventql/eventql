/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "fnord-fts/fts.h"
#include "fnord-fts/fts_common.h"
#include "fnord-fts/search/DisjunctionMaxQuery.h"
#include "SearchQuery.h"

using namespace fnord;

namespace cm {

SearchQuery::SearchQuery() {
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
    fnord::Language lang,
    fnord::fts::Analyzer* analyzer) {
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
  auto hits = results_->topDocs()->scoreDocs;

  fnord::iputs("return $0 docs", results_->getTotalHits());
  for (int32_t i = 0; i < hits.size(); ++i) {
    auto doc = searcher->doc(hits[i]->doc);
    fnord::WString docid_w = doc->get(L"_docid");
    String docid = StringUtil::convertUTF16To8(docid_w);
    fnord::iputs("doc $0 -> $1, $2", i, hits[i]->doc, docid);
  }
}

}
