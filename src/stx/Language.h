/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_LANGUAGE_H
#define _STX_LANGUAGE_H
#include "stx/stdtypes.h"

namespace stx {

/* ISO 639-1 */
enum class Language : uint16_t {
  UNKNOWN = 0,
  DE = 2,
  EN = 1,
  ES = 3,
  FR = 4,
  IT = 5,
  NL = 6,
  PL = 7
};

const uint16_t kMaxLanguage = 7;

Language languageFromString(const String& string);
String languageToString(Language lang);

} // namespace stx

#endif
