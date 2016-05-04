/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_FTS_STOPWORDDICTIONARY_H
#define _FNORD_FTS_STOPWORDDICTIONARY_H
#include "stx/stdtypes.h"
#include "stx/Language.h"

namespace stx {
namespace fts {

class StopwordDictionary {
public:

  StopwordDictionary();

  bool isStopword(Language lang, const stx::String& term) const;

  void addStopword(Language lang, const stx::String& term);
  void loadStopwordFile(const stx::String& filename);

protected:

  inline stx::String stopwordKey(Language lang, const stx::String& term) const {
    stx::String sw;
    sw += languageToString(lang);
    sw += "~";
    sw += term;
    return sw;
  }

  stx::Set<stx::String> stopwords_;
};

} // namespace fts
} // namespace stx

#endif
