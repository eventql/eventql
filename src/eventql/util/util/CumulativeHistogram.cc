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
#include "eventql/util/util/CumulativeHistogram.h"
#include "eventql/util/stringutil.h"

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

