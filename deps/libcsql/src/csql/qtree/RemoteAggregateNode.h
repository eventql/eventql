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

/**
 * Execute a group by/aggregation shard on a remote host. you must implement
 * execute_fn in such a way that it tunnels through to a call to
 * Runtime::executeAggregate on some remote node (i.e. the execute_fn needs to
 * return locally the value that was obtained by calling Runtime::executeAggregate
 * on some remote host)
 */
class RemoteAggregateNode : public TableExpressionNode {
public:
  typedef
      Function<ScopedPtr<InputStream> (const RemoteAggregateParams& params)>
      RemoteExecuteFn;

  RemoteAggregateNode(
      RefPtr<GroupByNode> group_by,
      RemoteExecuteFn execute_fn);

  Vector<RefPtr<SelectListNode>> selectList() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

  RemoteAggregateParams remoteAggregateParams() const;
  RemoteExecuteFn remoteExecuteFunction() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  Vector<TaskID> build(Transaction* txn, TaskDAG* tree) const override;

protected:
  RefPtr<GroupByNode> stmt_;
  RemoteExecuteFn execute_fn_;
};

} // namespace csql
