/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/UTF8.h>
#include "QueryAnalyzer.h"

using namespace fnord;

namespace cm {

QueryAnalyzer::QueryAnalyzer(
    StopwordDictionary* stopwords) :
    stopwords_(stopwords) {}

//String QueryAnalyzer::normalize(Language lang, const String& query) {}

void QueryAnalyzer::analyze(
    Language lang,
    const String& query,
    Set<String>* terms) {
  analyze(lang, query, [terms] (const String& term) {
    terms->emplace(term);
  });
}

void QueryAnalyzer::analyze(
    Language lang,
    const String& query,
    Function<void (const String& term)> term_callback) {
  tokenize(query, term_callback);
}

void QueryAnalyzer::stem(Language lang, String* term) const {
}

bool QueryAnalyzer::isStopword(Language lang, const String& term) const {
  return false;
}

void QueryAnalyzer::tokenize(
    const String& query,
    Function<void (const String& term)> term_callback) const {
  String buf;

  auto cur = query.c_str();
  auto end = cur + query.size();
  char32_t chr;
  while ((chr = UTF8::nextCodepoint(&cur, end)) != 0) {
    switch (chr) {

      /* token boundaries */
      case '!':
      case '"':
      case '#':
      case '$':
      case '%':
      case '&':
      case '\'':
      case '(':
      case ')':
      case '*':
      case '+':
      case ',':
      case '.':
      case '-':
      case '/':
      case ':':
      case ';':
      case '<':
      case '=':
      case '>':
      case '?':
      case '@':
      case '[':
      case '\\':
      case ']':
      case '^':
      case '_':
      case '`':
      case '{':
      case '|':
      case '}':
      case '~':
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        break;

      /* ignore chars */
      case L'✪':
        continue;

      /* valid chars */
      default:
        UTF8::encodeCodepoint(std::tolower(chr), &buf);
        continue;

    }

    if (buf.length() > 0) {
      term_callback(buf);
      buf.clear();
    }
  }
}

} // namespace cm

