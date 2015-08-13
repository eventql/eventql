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
#include <stx/autoref.h>

using namespace stx;

namespace csql {

class QueryTreeNode : public RefCounted {
public:

  QueryTreeNode() {}
  QueryTreeNode(const QueryTreeNode& other) = delete;
  QueryTreeNode& operator=(const QueryTreeNode& other) = delete;

  virtual RefPtr<QueryTreeNode> deepCopy() const = 0;

  template <typename T>
  RefPtr<T> deepCopyAs() const;

  size_t numChildren() const;

  RefPtr<QueryTreeNode> child(size_t index);

  RefPtr<QueryTreeNode>* mutableChild(size_t index);

  virtual String toString() const = 0;

protected:

  void addChild(RefPtr<QueryTreeNode>* table);

  Vector<RefPtr<QueryTreeNode>*> children_;
};

template <typename T>
RefPtr<T> QueryTreeNode::deepCopyAs() const {
  return deepCopy().asInstanceOf<T>();
}

} // namespace csql
