/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_AUTOCOMPLETEMODEL_H
#define _CM_AUTOCOMPLETEMODEL_H
#include "stx/json/json.h"
#include "ModelCache.h"
#include "zbase/TermInfo.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include <fnord-fts/Analyzer.h>

using namespace stx;

namespace cm {

struct AutoCompleteResult {
  String text;
  String url;
  double score;
  HashMap<String, String> attrs;
};


class AutoCompleteModel : public RefCounted {
public:
  typedef Vector<AutoCompleteResult> ResultListType;

  static RefPtr<AutoCompleteModel> fromCache(
      const String& customer,
      ModelCache* cache);

  AutoCompleteModel(
      const String& filename,
      RefPtr<fts::Analyzer> analyzer);

  ~AutoCompleteModel();

  void suggest(
      Language lang,
      const String& qstr,
      ResultListType* results);

protected:

  void suggestSingleTerm(
      Language lang,
      Vector<String> terms,
      ResultListType* results);

  void suggestMultiTerm(
      Language lang,
      Vector<String> terms,
      const Vector<String>& valid_terms,
      ResultListType* results);

  void loadModelFile(const String& filename);
  void addTermInfo(const String& term, const TermInfo& ti);

  RefPtr<fts::Analyzer> analyzer_;
  OrderedMap<String, SortedTermInfo> term_info_;
  HashMap<String, String> cat_names_;
};

}
#endif
