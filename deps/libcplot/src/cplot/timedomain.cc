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
#include "eventql/util/UnixTime.h"
#include "cplot/timedomain.h"

namespace util {
namespace chart {

TimeDomain::TimeDomain(
    util::UnixTime min_value,
    util::UnixTime max_value,
    bool is_logarithmic,
    bool is_inverted) :
    ContinuousDomain<util::UnixTime>(
        min_value,
        max_value,
        is_logarithmic,
        is_inverted) {}

std::string TimeDomain::label(util::UnixTime value) const {
  auto range = ContinuousDomain<util::UnixTime>::getRange();

  if (range < 60 * 60) {
    return value.toString("%H:%M:%S");
  } else if (range < 60 * 60 * 24) {
    return value.toString("%H:%M");
  } else {
    return value.toString("%Y-%m-%d %H:%M");
  }
}

}
}
