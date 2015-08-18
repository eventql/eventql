/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TERMINFO_H
#define _CM_TERMINFO_H
#include <inventory/ItemRef.h>


using namespace stx;

namespace cm {

struct TermInfo {
  TermInfo() : score(0.0f) {}

  double score;
  HashMap<String, uint64_t> related_terms;
  HashMap<String, double> top_categories;

  void merge(const TermInfo& other);

};

struct SortedTermInfo {
  SortedTermInfo(const TermInfo& ti);
  double score;
  Vector<Pair<String, uint64_t>> related_terms;
  Vector<Pair<String, double>> top_categories;
};

} // namespace cm

#endif
