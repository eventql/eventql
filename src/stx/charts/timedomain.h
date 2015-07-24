/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_TIMEDOMAIN_H
#define _FNORDMETRIC_TIMEDOMAIN_H
#include "stx/UnixTime.h"
#include "stx/charts/continuousdomain.h"

namespace fnord {
namespace chart {

class TimeDomain : public ContinuousDomain<fnord::UnixTime> {
public:

  TimeDomain(
    fnord::UnixTime min_value =
        std::numeric_limits<fnord::UnixTime>::max(),
    fnord::UnixTime max_value =
        std::numeric_limits<fnord::UnixTime>::min(),
    bool is_logarithmic = false,
    bool is_inverted = false);

  std::string label(fnord::UnixTime value) const;

};

}
}
#endif
