/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "TermInfo.h"

using namespace fnord;

namespace cm {

void TermInfo::merge(const TermInfo& other) {
  score += other.score;

  for (const auto& r: other.related_terms) {
    related_terms[r.first] += r.second;
  }

  for (const auto& r: other.top_categories) {
    top_categories[r.first] += r.second;
  }
}

} // namespace cm

