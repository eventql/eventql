/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/util/exception.h"
#include "eventql/util/Language.h"
#include "eventql/util/stringutil.h"

namespace util {

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

} // namespace util
