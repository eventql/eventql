/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/qtree/JoinNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>

using namespace stx;

namespace csql {

JoinNode::JoinNode(
    JoinType join_type,
    RefPtr<QueryTreeNode> base_table,
    RefPtr<QueryTreeNode> joined_table,
    Vector<RefPtr<SelectListNode>> select_list,
    Option<RefPtr<ValueExpressionNode>> where_expr,
    Option<RefPtr<ValueExpressionNode>> join_cond) :
    join_type_(join_type),
    base_table_(base_table),
    joined_table_(joined_table),
    select_list_(select_list),
    where_expr_(where_expr),
    join_cond_(join_cond) {
  for (const auto& sl : select_list_) {
    column_names_.emplace_back(sl->columnName());
  }

  addChild(&base_table_);
  addChild(&joined_table_);
}

JoinNode::JoinNode(
    const JoinNode& other) :
    join_type_(other.join_type_),
    base_table_(other.base_table_->deepCopy()),
    joined_table_(other.joined_table_->deepCopy()),
    input_map_(other.input_map_),
    column_names_(other.column_names_),
    join_cond_(other.join_cond_) {
  for (const auto& e : other.select_list_) {
    select_list_.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  if (!other.where_expr_.isEmpty()) {
    where_expr_ = Some(
        other.where_expr_.get()->deepCopyAs<ValueExpressionNode>());
  }

  addChild(&base_table_);
  addChild(&joined_table_);
}

JoinType JoinNode::joinType() const {
  return join_type_;
}

RefPtr<QueryTreeNode> JoinNode::baseTable() const {
  return base_table_;
}

RefPtr<QueryTreeNode> JoinNode::joinedTable() const {
  return joined_table_;
}

Vector<RefPtr<SelectListNode>> JoinNode::selectList() const {
  return select_list_;
}

Vector<String> JoinNode::outputColumns() const {
  return column_names_;
}

Vector<QualifiedColumn> JoinNode::allColumns() const {
  Vector<QualifiedColumn> cols;

  for (const auto& c :
      base_table_.asInstanceOf<TableExpressionNode>()->allColumns()) {
    cols.emplace_back(c);
  }

  for (const auto& c :
      joined_table_.asInstanceOf<TableExpressionNode>()->allColumns()) {
    cols.emplace_back(c);
  }

  return cols;
}

size_t JoinNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (int i = 0; i < column_names_.size(); ++i) {
    if (column_names_[i] == column_name) {
      return i;
    }
  }

  auto input_idx = getInputColumnIndex(column_name);
  if (input_idx != size_t(-1)) {
    auto slnode = new SelectListNode(new ColumnReferenceNode(input_idx));
    slnode->setAlias(column_name);
    select_list_.emplace_back(slnode);
    return select_list_.size() - 1;
  }

  return -1; // FIXME
}

size_t JoinNode::getInputColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (int i = 0; i < input_map_.size(); ++i) {
    if (input_map_[i].column == column_name) {
      return i;
    }
  }

  auto base_table_idx = base_table_
      .asInstanceOf<TableExpressionNode>()
      ->getColumnIndex(column_name, allow_add);

  auto joined_table_idx = joined_table_
      .asInstanceOf<TableExpressionNode>()
      ->getColumnIndex(column_name, allow_add);

  if (base_table_idx != size_t(-1) && joined_table_idx != size_t(-1)) {
    RAISEF(
        kRuntimeError,
        "ambiguous column reference: '$0'",
        column_name);
  }

  if (base_table_idx != size_t(-1)) {
    input_map_.emplace_back(InputColumnRef{
      .column = column_name,
      .table_idx = 0,
      .column_idx = base_table_idx
    });

    return input_map_.size() - 1;
  }

  if (joined_table_idx != size_t(-1)) {
    input_map_.emplace_back(InputColumnRef{
      .column = column_name,
      .table_idx = 1,
      .column_idx = joined_table_idx
    });

    return input_map_.size() - 1;
  }

  return -1;
}

Option<RefPtr<ValueExpressionNode>> JoinNode::whereExpression() const {
  return where_expr_;
}

Option<RefPtr<ValueExpressionNode>> JoinNode::joinCondition() const {
  return join_cond_;
}

RefPtr<QueryTreeNode> JoinNode::deepCopy() const {
  return new JoinNode(*this);
}

const Vector<JoinNode::InputColumnRef>& JoinNode::inputColumnMap() const {
  return input_map_;
}

String JoinNode::toString() const {
  auto str = StringUtil::format(
      "(join (base-table $0) (joined-table $0) (select-list",
      base_table_->toString(),
      joined_table_->toString());

  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }
  str += ")";

  if (!where_expr_.isEmpty()) {
    str += StringUtil::format(" (where $0)", where_expr_.get()->toString());
  }

  if (!join_cond_.isEmpty()) {
    str += StringUtil::format(" (join-cond $0)", join_cond_.get()->toString());
  }

  str += ")";
  return str;
};

void JoinNode::encode(
    QueryTreeCoder* coder,
    const JoinNode& node,
    stx::OutputStream* os) {
  os->appendUInt8((uint8_t) node.join_type_);

  coder->encode(node.base_table_.get(), os);
  coder->encode(node.joined_table_.get(), os);

  os->appendVarUInt(node.select_list_.size());
  for (const auto& e : node.select_list_) {
    coder->encode(e.get(), os);
  }

  if (!node.where_expr_.isEmpty()) {
    os->appendUInt8(1);
    coder->encode(node.where_expr_.get().get(), os);
  } else {
    os->appendUInt8(0);
  }

  if (!node.join_cond_.isEmpty()) {
    os->appendUInt8(1);
    coder->encode(node.join_cond_.get().get(), os);
  } else {
    os->appendUInt8(0);
  }
}

RefPtr<QueryTreeNode> JoinNode::decode (
    QueryTreeCoder* coder,
    stx::InputStream* is) {
  auto join_type = (JoinType) is->readUInt8();
  auto base_tbl = coder->decode(is);
  auto joined_tbl = coder->decode(is);

  Vector<RefPtr<SelectListNode>> select_list;
  auto select_list_size = is->readVarUInt();
  for (auto i = 0; i < select_list_size; ++i) {
    select_list.emplace_back(coder->decode(is).asInstanceOf<SelectListNode>());
  }

  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (is->readUInt8()) {
    where_expr = coder->decode(is).asInstanceOf<ValueExpressionNode>();
  }

  Option<RefPtr<ValueExpressionNode>> join_cond;
  if (is->readUInt8()) {
    join_cond = coder->decode(is).asInstanceOf<ValueExpressionNode>();
  }

  return new JoinNode(
      join_type,
      base_tbl,
      joined_tbl,
      select_list,
      where_expr,
      join_cond);
}

} // namespace csql

