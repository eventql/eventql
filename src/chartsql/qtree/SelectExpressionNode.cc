/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/SelectExpressionNode.h>

using namespace fnord;

namespace csql {

SelectExpressionNode::SelectExpressionNode(
    Vector<RefPtr<SelectListNode>> select_list) :
    select_list_(select_list) {}

Vector<RefPtr<SelectListNode>> SelectExpressionNode::selectList()
    const {
  return select_list_;
}

} // namespace csql
