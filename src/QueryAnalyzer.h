/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_QUERYANALYZER_H
#define _CM_QUERYANALYZER_H
#include "fnord-base/stdtypes.h"
#include "common.h"
#include "StopwordDictionary.h"

using namespace fnord;

namespace cm {

class QueryAnalyzer {
public:

  QueryAnalyzer(StopwordDictionary* stopwords);

  String normalize(Language lang, const String& query);

  void analyze(
      Language lang,
      const String& query,
      Set<String>* terms);

  void analyze(
      Language lang,
      const String& query,
      Function<void (const String& term)> term_callback);

  void tokenize(
      const String& query,
      Function<void (const String& term)> term_callback) const;

  void stem(Language lang, String* term) const;

  bool isStopword(Language lang, const String& term) const;

protected:
  StopwordDictionary* stopwords_;
};

} // namespace cm

#endif
