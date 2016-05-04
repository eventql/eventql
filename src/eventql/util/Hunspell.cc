/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/inspect.h"
#include "eventql/util/Hunspell.h"

namespace stx {
namespace fts {

Hunspell::Hunspell(
    const String& aff_file,
    const String& dict_file,
    const String& hyphen_file) {
  handle_ = Hunspell_create(aff_file.c_str(), dict_file.c_str());
  hyphen_dict_ = hnj_hyphen_load(hyphen_file.c_str());

  if (handle_ == nullptr || hyphen_dict_ == nullptr) {
    RAISE(kRuntimeError, "error initializing libhunspell");
  }
}

Hunspell::~Hunspell () {
  //Hunspell_destroy(handle_);
  //hnj_hyphen_free(hyphen_dict_);
}

Option<String> Hunspell::stem(const String& term) {
  char** res;
  Option<String> opt;

  auto cnt = Hunspell_stem(handle_, &res, term.c_str());
  if (cnt > 0) {
    opt = Some(String(res[cnt - 1]));
  }

  Hunspell_free_list(handle_, &res, cnt);
  return opt;
}

Vector<size_t> Hunspell::hyphenate(const String& term) {
  Vector<size_t> positions;

  Buffer buf(term.size() + 5); // libhunspell needs a fixed 5 bytes + strsize
  char* hyphens = (char*) buf.data();

  char** rep = NULL;
  int* pos = NULL;
  int* cut = NULL;
  auto res = hnj_hyphen_hyphenate2(
      hyphen_dict_,
      term.data(),
      term.size(),
      hyphens,
      NULL,
      &rep,
      &pos,
      &cut);

  if (res != 0) {
    return positions;
  }

  for (int i = 0; i < term.length(); ++i) {
    if (hyphens[i] & 1) {
      positions.emplace_back(i);
    }
  }

  return positions;
}


} // namespace fts
} // namespace stx
