/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/CallExpressionNode.h>

using namespace fnord;

namespace csql {

CallExpressionNode::CallExpressionNode(
    const String& symbol,
    Vector<RefPtr<ValueExpressionNode>> arguments) :
    symbol_(symbol),
    arguments_(arguments) {}

Vector<RefPtr<ValueExpressionNode>> CallExpressionNode::arguments() const {
  return arguments_;
}

const String& CallExpressionNode::symbol() const {
  return symbol_;
}

} // namespace csql

