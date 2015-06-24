/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/BuiltinExpressionNode.h>

using namespace fnord;

namespace csql {

BuiltinExpressionNode::BuiltinExpressionNode(
    const String& symbol,
    Vector<RefPtr<ScalarExpressionNode>> arguments) :
    symbol_(symbol),
    arguments_(arguments) {}


} // namespace csql

