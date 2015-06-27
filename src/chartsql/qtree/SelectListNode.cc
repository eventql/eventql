/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/SelectListNode.h>

using namespace fnord;

namespace csql {

SelectListNode::SelectListNode(
    RefPtr<ScalarExpressionNode> expr) :
    expr_(expr) {}

RefPtr<ScalarExpressionNode> SelectListNode::expression() const {
  return expr_;
}

} // namespace csql
