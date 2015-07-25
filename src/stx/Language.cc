/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/exception.h"
#include "stx/Language.h"
#include "stx/stringutil.h"

namespace stx {

Language languageFromString(const String& string) {
  String s(string);
  StringUtil::toLower(&s);

  if (s == "de") return Language::DE;
  if (s == "en") return Language::EN;
  if (s == "es") return Language::ES;
  if (s == "fr") return Language::FR;
  if (s == "it") return Language::IT;
  if (s == "nl") return Language::NL;
  if (s == "pl") return Language::PL;

  return Language::UNKNOWN;
}

String languageToString(Language lang) {
  switch (lang) {
    case Language::UNKNOWN: return "unknown";
    case Language::DE: return "de";
    case Language::EN: return "en";
    case Language::ES: return "es";
    case Language::FR: return "fr";
    case Language::IT: return "it";
    case Language::NL: return "nl";
    case Language::PL: return "pl";
  }
}

} // namespace stx
