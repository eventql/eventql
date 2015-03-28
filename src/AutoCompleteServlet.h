/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_AUTOCOMPLETESERVLET_H
#define _CM_AUTOCOMPLETESERVLET_H
#include "fnord-http/httpservice.h"
#include "fnord-json/json.h"
#include "reports/TermInfo.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include <fnord-fts/Analyzer.h>

using namespace fnord;

namespace cm {

/**
 *  GET /autocomplete
 *
 */
class AutoCompleteServlet : public fnord::http::HTTPService {
public:

  AutoCompleteServlet(RefPtr<fts::Analyzer> analyzer);

  void handleHTTPRequest(
      fnord::http::HTTPRequest* req,
      fnord::http::HTTPResponse* res);

  void addTermInfo(const String& term, const TermInfo& ti);

protected:

  typedef Vector<Tuple<String, uint64_t, String>> ResultListType;

  void suggestSingleTerm(
      Language lang,
      Vector<String> terms,
      ResultListType* results);

  void suggestMultiTerm(
      Language lang,
      Vector<String> terms,
      const Vector<String>& valid_terms,
      ResultListType* results);

  void suggestFuzzy(
      Language lang,
      Vector<String> terms,
      ResultListType* results);

  RefPtr<fts::Analyzer> analyzer_;
  OrderedMap<String, SortedTermInfo> term_info_;
};

}
#endif
