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
#include <stx/io/inputstream.h>
#include <csql/qtree/TableExpressionNode.h>
#include <csql/qtree/GroupByNode.h>
#include <csql/qtree/RemoteAggregateParams.pb.h>

using namespace stx;

namespace csql {

class RemoteAggregateNode : public QueryTreeNode {
public:
  typedef
      Function<ScopedPtr<InputStream> (const RemoteAggregateParams& params)>
      RemoteExecuteFn;

  RemoteAggregateNode(
      RefPtr<GroupByNode> group_by,
      RemoteExecuteFn execute_fn);

  Vector<RefPtr<SelectListNode>> selectList() const;

  RemoteAggregateParams remoteAggregateParams() const;
  RemoteExecuteFn remoteExecuteFunction() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

protected:
  RefPtr<GroupByNode> stmt_;
  RemoteExecuteFn execute_fn_;
};

} // namespace csql
