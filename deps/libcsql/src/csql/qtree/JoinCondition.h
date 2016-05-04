/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <csql/svalue.h>
#include <csql/qtree/TableExpressionNode.h>
#include <csql/qtree/ValueExpressionNode.h>
#include <csql/qtree/SelectListNode.h>

using namespace stx;

namespace csql {

class JoinCondition : public RefCounted {
public:

  virtual RefPtr<ValueExpressionNode> getExpression() const = 0;

};

class ExpressionJoinCondition : public  JoinCondition {
public:

  ExpressionJoinCondition(RefPtr<ValueExpressionNode> expr);

  RefPtr<ValueExpressionNode> getExpression() const override;

protected:
  RefPtr<ValueExpressionNode> expr_;
};

class ColumnListJoinCondition : public JoinCondition {
public:

  RefPtr<ValueExpressionNode> getExpression() const override;

};

} // namespace csql
