/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_CUMULATIVEHISTOGRAM_H
#define _FNORD_UTIL_CUMULATIVEHISTOGRAM_H
#include "fnord-base/stdtypes.h"

namespace fnord {
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
}

#endif
