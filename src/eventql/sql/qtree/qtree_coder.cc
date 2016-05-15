/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
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
#include <eventql/sql/qtree/SelectExpressionNode.h>
#include <eventql/sql/qtree/JoinNode.h>
#include <eventql/sql/qtree/SubqueryNode.h>

using namespace stx;

namespace csql {

QueryTreeCoder::QueryTreeCoder(Transaction* txn) : txn_(txn) {
  registerType<CallExpressionNode>(1);
  registerType<ColumnReferenceNode>(2);
  registerType<DescribeTableNode>(3);
  registerType<GroupByNode>(4);
  registerType<IfExpressionNode>(5);
  registerType<JoinNode>(6);
  registerType<LimitNode>(7);
  registerType<LiteralExpressionNode>(8);
  registerType<OrderByNode>(9);
  registerType<RegexExpressionNode>(10);
  registerType<SelectExpressionNode>(11);
  registerType<SelectListNode>(12);
  registerType<SequentialScanNode>(13);
  registerType<ShowTablesNode>(14);
  registerType<SubqueryNode>(15);
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

