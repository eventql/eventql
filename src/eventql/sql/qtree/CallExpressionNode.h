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
#include <eventql/util/stdtypes.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/qtree_coder.h>

using namespace stx;

namespace csql {

class CallExpressionNode : public ValueExpressionNode {
public:

  CallExpressionNode(
      const String& symbol,
      Vector<RefPtr<ValueExpressionNode>> arguments);

  Vector<RefPtr<ValueExpressionNode>> arguments() const override;

  const String& symbol() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toSQL() const override;

  static void encode(
      QueryTreeCoder* coder,
      const CallExpressionNode& node,
      stx::OutputStream* os);

  static RefPtr<QueryTreeNode> decode (
      QueryTreeCoder* coder,
      stx::InputStream* is);

protected:
  String symbol_;
  Vector<RefPtr<ValueExpressionNode>> arguments_;
};

} // namespace csql
