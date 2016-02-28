/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_FTS_GERMANSTEMMER_H
#define _FNORD_FTS_GERMANSTEMMER_H
#include "stx/stdtypes.h"
#include "zbase/util/Stemmer.h"
#include "zbase/util/Hunspell.h"
#include "zbase/util/SynonymDictionary.h"

namespace stx {
namespace fts {

class GermanStemmer : public Stemmer {
public:

  GermanStemmer(
      const stx::String& hunspell_aff_file,
      const stx::String& hunspell_dict_file,
      const stx::String& hunspell_hyphen_file,
      SynonymDictionary* synonyms);

  void stem(Language lang, stx::String* term) override;
  Option<stx::String> findStemFor(Language lang, const stx::String& term);

  void removeUmlauts(stx::String* term);

protected:
  void stemWithUmlauts(Language lang, stx::String* term);
  Hunspell hunspell_;
  SynonymDictionary* synonyms_;
};

} // namespace fts
} // namespace stx

#endif
