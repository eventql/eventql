/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "zbase/util/SynonymDictionary.h"

namespace stx {
namespace fts {

Option<stx::String> SynonymDictionary::lookup(Language lang, const stx::String& term) {
  auto iter = synonyms_.find(synonymKey(lang, term));
  if (iter == synonyms_.end()) {
    return None<stx::String>();
  } else {
    return Some(iter->second);
  }
}

void SynonymDictionary::addSynonym(
    Language lang,
    const stx::String& term,
    const stx::String& canonical_term) {
  synonyms_[synonymKey(lang, term)] = canonical_term;
}

void SynonymDictionary::loadSynonmFile(const stx::String& filename) {
}

} // namespace fts
} // namespace stx
