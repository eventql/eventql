/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/util/CumulativeHistogram.h"
#include "stx/stringutil.h"

namespace stx {
namespace util {

CumulativeHistogram CumulativeHistogram::withLinearBins(double bin_size) {
  return CumulativeHistogram(
      [bin_size] (double value) { return value / bin_size; },
      [bin_size] (size_t idx) { return idx * bin_size; });
}

CumulativeHistogram::CumulativeHistogram(
    Function<size_t (double value)> bin_fn,
    Function<double (size_t bin_idx)> bin_lowerb_fn) :
    bin_fn_(bin_fn),
    bin_lowerb_fn_(bin_lowerb_fn),
    cumul_(0) {}

void CumulativeHistogram::addDatum(double value, double num_observations) {
  auto bin = bin_fn_(value);

  while (bins_.size() < (bin + 1)) {
    bins_.emplace_back(0);
  }

  bins_[bin] += num_observations;
  cumul_ += num_observations;
}

Vector<Pair<String, double>>
    CumulativeHistogram::cumulativeRelativeHistogram() {
  double c = 0;

  Vector<Pair<String, double>> res;
  for (int i = 0; i < bins_.size(); ++i) {
    c += bins_[i];

    res.emplace_back(
        StringUtil::toString(bin_lowerb_fn_(i + 1)),
        c / cumul_);
  }

  return res;
}

}
}

