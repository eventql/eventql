/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_FTS_HUNSPELL_H
#define _FNORD_FTS_HUNSPELL_H
#include "stx/stdtypes.h"
#include "stx/option.h"
#include "hunspell/hunspell.h"
#include "hunspell/hyphen.h"

namespace stx {
namespace fts {

class Hunspell {
public:

  Hunspell(
      const String& aff_file,
      const String& dict_file,
      const String& hyphen_file);

  ~Hunspell();

  Option<String> stem(const String& term);

  Vector<size_t> hyphenate(const String& term);

protected:
  Hunhandle* handle_;
  HyphenDict* hyphen_dict_;
};

} // namespace fts
} // namespace stx

#endif
