/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/io/fileutil.h>
#include <fnord-fts/GermanStemmer.h>
#include <fnord-fts/StopwordDictionary.h>
#include <fnord-fts/SynonymDictionary.h>
#include "Analyzer.h"

using namespace fnord;

namespace cm {

Analyzer::Analyzer(
    const String& cmconf) :
    german_stemmer_(
        FileUtil::joinPaths(cmconf, "hunspell_de.aff"),
        FileUtil::joinPaths(cmconf, "hunspell_de.dic"),
        FileUtil::joinPaths(cmconf, "hunspell_de.hyphen"),
        &synonyms_) {
  stopwords_.loadStopwordFile(
      FileUtil::joinPaths(cmconf, "stopwords.txt"));

  synonyms_.addSynonym(Language::DE, "mützen", "mütze");
  synonyms_.addSynonym(Language::DE, "bänder", "band");
  synonyms_.addSynonym(Language::DE, "girlanden", "girlande");
}

void Analyzer::extractTerms(
    Language lang,
    const String& query,
    Function<void (const String& term)> term_callback) {
  tokenizer_.tokenize(query, [this, lang, term_callback] (const String& t) {
    if (stopwords_.isStopword(lang, t)) {
      return;
    }

    String term(t);
    stem(lang, &term);

    term_callback(term);
  });
}

void Analyzer::extractTerms(
    Language lang,
    const String& query,
    Set<String>* terms) {
  extractTerms(lang, query, [terms] (const String& term) {
    terms->emplace(term);
  });
}

void Analyzer::stem(Language lang, String* term) {
  switch (lang) {

    case Language::DE:
      german_stemmer_.stem(lang, term);
      break;

    default:
      break;

  }
}

}
