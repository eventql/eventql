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
#ifndef _STX_UTIL_CUMULATIVEHISTOGRAM_H
#define _STX_UTIL_CUMULATIVEHISTOGRAM_H
#include "eventql/util/stdtypes.h"

namespace util {

class CumulativeHistogram {
public:

  static CumulativeHistogram withLinearBins(double bin_size);

  void addDatum(double value, double num_observations);

  Vector<Pair<String, double>> absoluteHistogram();
  Vector<Pair<String, double>> relativeHistogram();
  Vector<Pair<String, double>> cumulativeAbsoluteHistogram();
  Vector<Pair<String, double>> cumulativeRelativeHistogram();

protected:
  CumulativeHistogram(
      Function<size_t (double value)> bin_fn,
      Function<double (size_t bin_idx)> bin_lowerb_fn);

  Function<size_t (double value)> bin_fn_;
  Function<double (size_t bin_idx)> bin_lowerb_fn_;
  double cumul_;
  Vector<double> bins_;
};

}

#endif
