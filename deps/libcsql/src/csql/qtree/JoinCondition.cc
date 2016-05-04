/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/JoinCondition.h>

using namespace stx;

namespace csql {

ExpressionJoinCondition::ExpressionJoinCondition(
    RefPtr<ValueExpressionNode> expr) :
    expr_(expr) {}

RefPtr<ValueExpressionNode> ExpressionJoinCondition::getExpression() const {
  return expr_;
}

RefPtr<ValueExpressionNode> ColumnListJoinCondition::getExpression() const {
  RAISE(kNotYetImplementedError);
}

} // namespace csql
