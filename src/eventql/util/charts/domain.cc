/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/util/charts/domain.h"
#include "eventql/util/charts/continuousdomain.h"
#include "eventql/util/charts/discretedomain.h"
#include "eventql/util/charts/timedomain.h"

#include "eventql/eventql.h"
namespace util {
namespace chart {

const char AnyDomain::kDimensionLetters[] = "xyz";
const int AnyDomain::kDefaultNumTicks = 8;
const double AnyDomain::kDefaultDomainPadding = 0.1;

template <> Domain<int64_t>*
    Domain<int64_t>::mkDomain() {
  return new ContinuousDomain<int64_t>();
}

template <> Domain<double>*
    Domain<double>::mkDomain() {
  return new ContinuousDomain<double>();
}

template <> Domain<UnixTime>*
    Domain<UnixTime>::mkDomain() {
  return new TimeDomain();
}

template <> Domain<std::string>* Domain<std::string>::mkDomain() {
  return new DiscreteDomain<std::string>();
}

}
}

