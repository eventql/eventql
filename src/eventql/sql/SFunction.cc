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
#include <eventql/sql/SFunction.h>

#include "eventql/eventql.h"

namespace csql {

PureFunction::PureFunction() : call(nullptr), has_side_effects(false) {}

PureFunction::PureFunction(
    void (*_call)(sql_txn* ctx, int argc, SValue* in, SValue* out),
    bool _has_side_effects) :
    call(_call),
    has_side_effects(_has_side_effects) {}

SFunction::SFunction() :
    type(FN_PURE),
    vtable{ .t_pure = nullptr } {}

SFunction::SFunction(
    PureFunction fn) :
    type(FN_PURE),
    vtable{ .t_pure = fn } {}

SFunction::SFunction(
    AggregateFunction fn) :
    type(FN_AGGREGATE),
    vtable{ .t_aggregate = fn } {}

bool SFunction::isAggregate() const {
  switch (type) {
    case FN_PURE: return false;
    case FN_AGGREGATE: return true;
  }
}

bool SFunction::hasSideEffects() const {
  switch (type) {
    case FN_AGGREGATE:
      return false;
    case FN_PURE:
      return vtable.t_pure.has_side_effects;
  }
}


}
