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
#include <fnord/autoref.h>
#include <fnord/option.h>
#include <chartsql/qtree/QueryTreeNode.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {
class DefaultRuntime;

class TableExpressionBuilder {
public:

  struct BuildRule : public RefCounted {
    virtual Option<ScopedPtr<TableExpression>> build(
          RefPtr<TableExpressionNode> node,
          DefaultRuntime* runtime) const = 0;
  };

  ScopedPtr<TableExpression> build(
      RefPtr<TableExpressionNode> node,
      DefaultRuntime* runtime);

  void addBuildRule(RefPtr<BuildRule> rule);

protected:
  Vector<RefPtr<BuildRule>> rules_;
};

} // namespace csql
