/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <unistd.h>
#include "common.h"

namespace cm {

/**
 * mandatory params:
 *  v    -- pixel ver.  -- value: 1
 *  c    -- clickid     -- format "<uid>~<eventid>", e.g. "f97650cb~b28c61d5c"
 *  e    -- eventtype   -- format "{q,v}" (query, visit)
 *
 * params for eventtype=q (query):
 *  is   -- item ids    -- format "<setid>~<itemid>~<pos>,..."
 *
 * params for eventtype=v (visit):
 *  i    -- itemid      -- format "<setid>~<itemid>"
 *
 */
bool isReservedPixelParam(const std::string p) {
  return p == "c" || p == "e" || p == "i" || p == "is" || p == "v";
}

std::string cmHostname() {
  char hostname[128];
  gethostname(hostname, sizeof(hostname));
  hostname[127] = 0;
  return std::string(hostname);
}

Option<String> extractAttr(const Vector<String>& attrs, const String& attr) {
  auto prefix = attr + ":";

  for (const auto& a : attrs) {
    if (StringUtil::beginsWith(a, prefix)) {
      return Some(a.substr(prefix.length()));
    }
  }

  return None<String>();
}

void tokenizeAndStem(
    Language lang,
    const WString& query,
    Function<void (const WString& token)> fn) {
  Vector<WString> parts(1);

  auto begin = query.c_str();
  auto end = begin + query.size();
  for (auto cur = begin; cur < end; ++cur) {
    switch (*cur) {

      /* token boundaries */
      case '!':
      case '"':
      case '#':
      case '$':
      case '%':
      case '&':
      case '\'':
      case '(':
      case ')':
      case '*':
      case '+':
      case ',':
      case '.':
      case '-':
      case '/':
      case ':':
      case ';':
      case '<':
      case '=':
      case '>':
      case '?':
      case '@':
      case '[':
      case '\\':
      case ']':
      case '^':
      case '_':
      case '`':
      case '{':
      case '|':
      case '}':
      case '~':
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        if (parts.back().length() > 0) {
          parts.emplace_back();
        }
        break;

      /* ignore chars */
      case L'✪':
        continue;

      /* valid chars */
      default:
        parts.back() += std::tolower(*cur);
        break;

    }
  }

  for (auto& p : parts) {
    if (p.length() == 0) {
      continue;
    }

    fn(p);
  }
}

void tokenizeAndStem(Language lang, const String& query, Set<String>* tokens) {
  tokenizeAndStem(
      lang,
      StringUtil::convertUTF8To16(query),
      [tokens] (const WString& token) {
        tokens->emplace(StringUtil::convertUTF16To8(token));
      });
}

String joinBagOfWords(const Set<String>& words) {
  Vector<String> v;

  for (const auto& w : words) {
    if (w.length() == 0) {
      continue;
    }

    v.emplace_back(w);
  }

  std::sort(v.begin(), v.end());

  return StringUtil::join(v, " ");
}

bool isItemEligible(
    ItemEligibility eligibility,
    const cm::JoinedQuery& query,
    const cm::JoinedQueryItem& item) {
  if (eligibility == ItemEligibility::DAWANDA_FIRST_EIGHT) {
    if (item.position < 5 || item.position > 12) {
      return false;
    }
  }

  return true;
}

}
