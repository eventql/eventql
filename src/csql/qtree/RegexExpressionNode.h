/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/qtree/ValueExpressionNode.h>

using namespace stx;

namespace csql {

class RegexExpressionNode : public ValueExpressionNode {
public:

  RegexExpressionNode(
      RefPtr<ValueExpressionNode> subject,
      const String& pattern);

  Vector<RefPtr<ValueExpressionNode>> arguments() const override;

  RefPtr<ValueExpressionNode> subject() const;

  const String& pattern() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toSQL() const override;

protected:
  RefPtr<ValueExpressionNode> subject_;
  String pattern_;
};

} // namespace csql
