/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/DefaultQueryPlan.h>

using namespace fnord;

namespace csql {

DefaultQueryPlan::DefaultQueryPlan(
    ScopedPtr<TableExpression> entry) :
    entry_(std::move(entry)) {}

void DefaultQueryPlan::execute(
    Function<bool (int argc, const SValue* argv)> fn) {

}

} // namespace csql
