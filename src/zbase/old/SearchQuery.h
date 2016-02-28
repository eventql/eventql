/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_SEARCHQUERY_H
#define _CM_SEARCHQUERY_H
#include "stx/json/json.h"
#include "zbase/util/fts.h"
#include "zbase/util/fts_common.h"
#include "zbase/util/Analyzer.h"
#include "IndexReader.h"

using namespace stx;

namespace zbase {

class SearchQuery {
public:

  SearchQuery();

  void addField(const String& field_name, double boost = 1.0);

  void addTerm(const String& term);

  void addQuery(
      const String& query,
      stx::Language lang,
      stx::fts::Analyzer* analyzer);

  void execute(IndexReader* index);

  void writeJSON(json::JSONOutputStream* target);

protected:
  struct FieldInfo {
    stx::WString field_name;
    double boost;
  };

  Vector<FieldInfo> fields_;
  stx::Set<String> terms_;
  stx::fts::TopScoreDocCollectorPtr results_;
  stx::Vector<String> res_docids_;
  size_t total_hits_;
};

}
#endif
