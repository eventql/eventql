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
#include <eventql/sql/extensions/chartsql/seriesadapter.h>
#include <eventql/util/exception.h>

namespace csql {

AnySeriesAdapter::AnySeriesAdapter(
    int name_ind,
    int x_ind,
    int y_ind,
    int z_ind) :
    name_ind_(name_ind),
    x_ind_(x_ind),
    y_ind_(y_ind),
    z_ind_(z_ind) {}

void AnySeriesAdapter::applyProperties(
    SValue* row,
    int row_len,
    util::chart::Series* series,
    util::chart::Series::AnyPoint* point) {
  for (const auto& prop : prop_indexes_) {
    if (prop.second >= row_len) {
      RAISE(kRuntimeError, "invalid index for property");
    }

    series->setProperty(prop.first, point, row[prop.second].getString());
  }
}


}
