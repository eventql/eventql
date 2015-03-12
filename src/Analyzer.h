/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALZE_H
#define _CM_ANALZE_H
#include <fnord-base/stdtypes.h>
#include "fnord-fts/SimpleTokenizer.h"
#include "fnord-fts/StopwordDictionary.h"
#include "fnord-fts/SynonymDictionary.h"
#include "fnord-fts/GermanStemmer.h"

using namespace fnord;

namespace cm {

class Analyzer {
public:

  Analyzer(const String& cmconf);

  void extractTerms(
      Language lang,
      const String& query,
      Set<String>* terms);

  void extractTerms(
      Language lang,
      const String& query,
      Function<void (const String& term)> term_callback);

  void stem(Language lang, String* term);

protected:

  fnord::fts::SimpleTokenizer tokenizer_;
  fnord::fts::StopwordDictionary stopwords_;
  fnord::fts::SynonymDictionary synonyms_;
  fnord::fts::GermanStemmer german_stemmer_;
};

}
#endif
