/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_FTS_QUERYANALYZER_H
#define _FNORD_FTS_QUERYANALYZER_H
#include "stx/stdtypes.h"
#include "stx/Language.h"
#include "fnord-fts/Stemmer.h"
#include "fnord-fts/StopwordDictionary.h"

namespace stx {
namespace fts {

class SimpleTokenizer {
public:

  void tokenize(
      const stx::String& query,
      Function<void (const stx::String& term)> term_callback) const;
};

} // namespace fts
} // namespace stx

#endif
