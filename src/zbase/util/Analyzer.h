/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_FTS_ANALYZER_H
#define _FNORD_FTS_ANALYZER_H
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include "zbase/util/SimpleTokenizer.h"
#include "zbase/util/StopwordDictionary.h"
#include "zbase/util/SynonymDictionary.h"
#include "zbase/util/GermanStemmer.h"

namespace stx {
namespace fts {

class Analyzer : public RefCounted {
public:

  Analyzer(const stx::String& conf);

  void extractTerms(
      Language lang,
      const stx::String& query,
      stx::Set<stx::String>* terms);

  void extractTerms(
      Language lang,
      const stx::String& query,
      Function<void (const stx::String& term)> term_callback);

  void stem(Language lang, stx::String* term);

  stx::String normalize(Language lang, const stx::String& query);

  void tokenize(
      Language lang,
      const stx::String& query,
      stx::Vector<stx::String>* terms);

protected:
  stx::fts::SimpleTokenizer tokenizer_;
  stx::fts::StopwordDictionary stopwords_;
  stx::fts::SynonymDictionary synonyms_;
  stx::fts::GermanStemmer german_stemmer_;
};

}
}
#endif
