/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/qtree/qtree_coder.h>
#include <eventql/sql/qtree/LimitNode.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/qtree/SelectListNode.h>
#include <eventql/sql/qtree/LiteralExpressionNode.h>
#include <eventql/sql/qtree/CallExpressionNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/sql/qtree/DescribeTableNode.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/qtree/ShowTablesNode.h>
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/IfExpressionNode.h>
#include <eventql/sql/qtree/RegexExpressionNode.h>

using namespace stx;

namespace csql {

QueryTreeCoder::QueryTreeCoder(Transaction* txn) : txn_(txn) {
  registerType<LimitNode>(1);
  registerType<SequentialScanNode>(2);
  registerType<SelectListNode>(3);
  registerType<LiteralExpressionNode>(4);
  registerType<CallExpressionNode>(5);
  registerType<ColumnReferenceNode>(6);
  registerType<DescribeTableNode>(7);
  registerType<OrderByNode>(8);
  registerType<ShowTablesNode>(9);
  registerType<GroupByNode>(9);
  registerType<IfExpressionNode>(10);
  registerType<RegexExpressionNode>(11);
}

void QueryTreeCoder::encode(RefPtr<QueryTreeNode> tree, stx::OutputStream* os) {
  auto coder = coders_by_type_id_.find(&typeid(*tree));
  if (coder == coders_by_type_id_.end()) {
    RAISEF(kIOError, "don't know how to encode this QueryTreeNode: $0", tree->toString());
  }

  os->appendVarUInt(coder->second.wire_type_id);
  coder->second.encode_fn(this, tree, os);
}

RefPtr<QueryTreeNode> QueryTreeCoder::decode(stx::InputStream* is) {
  auto wire_type = is->readVarUInt();

  auto coder = coders_by_wire_type_id_.find(wire_type);
  if (coder == coders_by_wire_type_id_.end()) {
    RAISEF(kIOError, "don't know how to decode this QueryTreeNode: $0", wire_type);
  }

  return coder->second.decode_fn(this, is);
}

void QueryTreeCoder::registerType(QueryTreeCoderType t) {
  coders_by_type_id_[t.type_id] = t;
  coders_by_wire_type_id_[t.wire_type_id] = t;
}

Transaction* QueryTreeCoder::getTransaction() const {
  return txn_;
}


} // namespace csql

