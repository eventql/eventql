/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include "TermInfo.h"

using namespace stx;

namespace cm {

void TermInfo::merge(const TermInfo& other) {
  score += other.score;

  for (const auto& r : other.related_terms) {
    related_terms[r.first] += r.second;
  }

  for (const auto& c : other.top_categories) {
    top_categories[c.first] += c.second;
  }
}

SortedTermInfo::SortedTermInfo(const TermInfo& ti) : score(ti.score) {
  for (const auto& r : ti.related_terms) {
    related_terms.emplace_back(r);
  }

  std::sort(related_terms.begin(), related_terms.end(), [] (
      const Pair<String, uint64_t>& a,
      const Pair<String, uint64_t>& b) {
    return b.second < a.second;
  });

  for (const auto& c : ti.top_categories) {
    top_categories.emplace_back(c);
  }

  std::sort(top_categories.begin(), top_categories.end(), [] (
      const Pair<String, uint64_t>& a,
      const Pair<String, uint64_t>& b) {
    return b.second < a.second;
  });
}

} // namespace cm

