/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-fts/FTSQuery.h"

namespace stx {
namespace fts {

FTSQuery::FTSQuery() {}

void FTSQuery::addField(const stx::String& field_name, double boost) {
  FieldInfo fi;
  fi.field_name = StringUtil::convertUTF8To16(field_name);
  fi.boost = boost;
  fields_.emplace_back(fi);
}

void FTSQuery::addTerm(const stx::String& term) {
  terms_.emplace(term);
}

void FTSQuery::addQuery(
    const stx::String& query,
    stx::Language lang,
    Analyzer* analyzer) {
  analyzer->extractTerms(lang, query, [this] (const stx::String& t) {
    addTerm(t);
  });
}

void FTSQuery::execute(IndexSearcher* searcher) {
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

  auto collector = fts::TopScoreDocCollector::create(
      500,
      false);

  searcher->search(query, collector);
  auto hits = collector->topDocs()->scoreDocs;

  stx::iputs("return $0 docs", collector->getTotalHits());
  for (int32_t i = 0; i < hits.size(); ++i) {
    auto doc = searcher->doc(hits[i]->doc);
    stx::WString docid_w = doc->get(L"_docid");
    stx::String docid = StringUtil::convertUTF16To8(docid_w);
    stx::iputs("doc $0 -> $1, $2", i, hits[i]->doc, docid);
  }
}

}
}
