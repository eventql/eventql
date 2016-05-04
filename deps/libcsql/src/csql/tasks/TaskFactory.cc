/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/tasks/TaskFactory.h>
using namespace stx;

namespace csql {

SimpleTableExpressionFactory::SimpleTableExpressionFactory(
    FactoryFn factory_fn) :
    factory_fn_(factory_fn) {}

RefPtr<Task> SimpleTableExpressionFactory::build(
    Transaction* txn,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
  return factory_fn_(txn, std::move(input));
}

} // namespace csql
