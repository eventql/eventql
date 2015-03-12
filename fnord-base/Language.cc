/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-base/exception.h"
#include "fnord-base/Language.h"
#include "fnord-base/stringutil.h"

namespace fnord {

Language languageFromString(const String& string) {
  String s(string);
  StringUtil::toLower(&s);

  if (s == "en") return Language::EN;
  if (s == "de") return Language::DE;

  RAISEF(kRuntimeError, "invalid language: $0", string);
}

} // namespace fnord
