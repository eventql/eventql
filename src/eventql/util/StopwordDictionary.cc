/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "eventql/util/StopwordDictionary.h"
#include "stx/io/mmappedfile.h"

namespace stx {
namespace fts {

StopwordDictionary::StopwordDictionary() {}

bool StopwordDictionary::isStopword(Language lang, const String& term) const {
  return stopwords_.count(stopwordKey(lang, term)) != 0;
}

void StopwordDictionary::addStopword(Language lang, const String& term) {
  stopwords_.emplace(stopwordKey(lang, term));
}

void StopwordDictionary::loadStopwordFile(const String& filename) {
  io::MmappedFile mmap(File::openFile(filename, File::O_READ));

  char* cur = (char*) mmap.data();
  auto end = cur + mmap.size();
  auto begin = cur;

  for (; cur < end; ++cur) {
    if (*cur == '\n') {
      String line(begin, cur);
      auto sep = line.find(" ");
      if (sep == std::string::npos) {
        RAISEF(kRuntimeError, "invalid stopword file -- line: $0", line);
      }

      auto lang = line.substr(0, sep);
      auto term = line.substr(sep + 1);

      addStopword(languageFromString(lang), term);
      begin = cur + 1;
    }
  }
}

} // namespace fts
} // namespace stx
