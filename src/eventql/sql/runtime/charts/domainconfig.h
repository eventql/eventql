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
#ifndef _FNORDMETRIC_SQLEXTENSIONS_DOMAINCONFIG_H
#define _FNORDMETRIC_SQLEXTENSIONS_DOMAINCONFIG_H
#include <algorithm>
#include <stdlib.h>
#include <assert.h>
#include <cplot/domain.h>
#include <cplot/drawable.h>
#include <eventql/sql/svalue.h>

namespace csql {

class DomainConfig {
public:

  /**
   * @param dimension 0 for x, 1 for y, etc
   */
  DomainConfig(
      util::chart::Drawable* drawable,
      util::chart::AnyDomain::kDimension dimension);

  void setMin(const SValue& value);
  void setMax(const SValue& value);
  void setInvert(bool invert);
  void setLogarithmic(bool logarithmic);

protected:
  util::chart::AnyDomain* domain_;
  char dimension_letter_;
};

}
#endif
