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
#include <eventql/util/option.h>
#include <eventql/sql/qtree/QueryTreeNode.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/qtree_coder.h>

using namespace stx;

namespace csql {

class SelectListNode : public QueryTreeNode {
public:

  SelectListNode(RefPtr<ValueExpressionNode> expr);

  RefPtr<ValueExpressionNode> expression() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String columnName() const;

  void setAlias(const String& alias);

  String toString() const override;

  static void encode(
      QueryTreeCoder* coder,
      const SelectListNode& node,
      stx::OutputStream* os);

  static RefPtr<QueryTreeNode> decode(
      QueryTreeCoder* coder,
      stx::InputStream* is);

protected:
  Option<String> alias_;
  RefPtr<ValueExpressionNode> expr_;
};

} // namespace csql
