/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <stx/io/fileutil.h>
#include <eventql/util/GermanStemmer.h>
#include <eventql/util/StopwordDictionary.h>
#include <eventql/util/SynonymDictionary.h>
#include <eventql/util/Analyzer.h>

namespace stx {
namespace fts {

Analyzer::Analyzer(const stx::String& conf) :
    german_stemmer_(
        FileUtil::joinPaths(conf, "hunspell_de.aff"),
        FileUtil::joinPaths(conf, "hunspell_de.dic"),
        FileUtil::joinPaths(conf, "hunspell_de.hyphen"),
        &synonyms_) {
  stopwords_.loadStopwordFile(
      FileUtil::joinPaths(conf, "stopwords.txt"));
}

void Analyzer::extractTerms(
    Language lang,
    const stx::String& query,
    Function<void (const stx::String& term)> term_callback) {
  tokenizer_.tokenize(query, [this, lang, term_callback] (const stx::String& t) {
    if (stopwords_.isStopword(lang, t)) {
      return;
    }

    stx::String term(t);
    stem(lang, &term);

    term_callback(term);
  });
}

void Analyzer::extractTerms(
    Language lang,
    const stx::String& query,
    Set<stx::String>* terms) {
  extractTerms(lang, query, [terms] (const stx::String& term) {
    terms->emplace(term);
  });
}

void Analyzer::stem(Language lang, stx::String* term) {
  switch (lang) {

    case Language::DE:
      german_stemmer_.stem(lang, term);
      break;

    default:
      break;

  }
}

stx::String Analyzer::normalize(Language lang, const stx::String& query) {
  Vector<stx::String> terms;
  tokenize(lang, query, &terms);

  std::sort(terms.begin(), terms.end(), [] (
      const stx::String& a,
      const stx::String& b) {
    return a < b;
  });

  return StringUtil::join(terms, " ");
}

void Analyzer::tokenize(
      Language lang,
      const stx::String& query,
      stx::Vector<stx::String>* terms) {
  tokenizer_.tokenize(query, [this, lang, terms] (const stx::String& t) {
    if (t.length() == 0) {
      return;
    }

    if (stopwords_.isStopword(lang, t)) {
      return;
    }

    terms->emplace_back(t);
  });
}

}
}
