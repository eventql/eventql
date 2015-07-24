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
#include <fnord/stdtypes.h>
#include <chartsql/qtree/TableExpressionNode.h>

using namespace fnord;

namespace csql {

class ShowTablesNode : public TableExpressionNode {
public:

  Vector<RefPtr<QueryTreeNode>> inputTables() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

};

} // namespace csql
