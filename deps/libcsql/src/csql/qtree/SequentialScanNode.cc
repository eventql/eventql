/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/SequentialScanNode.h>
#include <csql/qtree/ColumnReferenceNode.h>
#include <csql/qtree/CallExpressionNode.h>
#include <csql/qtree/LiteralExpressionNode.h>
#include <csql/qtree/QueryTreeUtil.h>

using namespace stx;

namespace csql {

bool ScanConstraint::operator==(const ScanConstraint& other) const {
  return
      column_name == other.column_name &&
      type == other.type &&
      value == other.value;
}

bool ScanConstraint::operator!=(const ScanConstraint& other) const {
  return !(*this == other);
}

SequentialScanNode::SequentialScanNode(
    const TableInfo& table_info,
    Vector<RefPtr<SelectListNode>> select_list,
    Option<RefPtr<ValueExpressionNode>> where_expr) :
    SequentialScanNode(
        table_info,
        select_list,
        where_expr,
        AggregationStrategy::NO_AGGREGATION) {}

SequentialScanNode::SequentialScanNode(
    const TableInfo& table_info,
    Vector<RefPtr<SelectListNode>> select_list,
    Option<RefPtr<ValueExpressionNode>> where_expr,
    AggregationStrategy aggr_strategy) :
    table_name_(table_info.table_name),
    select_list_(select_list),
    where_expr_(where_expr),
    aggr_strategy_(aggr_strategy) {
  for (const auto& col : table_info.columns) {
    table_columns_.emplace_back(col.column_name);
  }

  if (!where_expr_.isEmpty()) {
    QueryTreeUtil::findConstraints(where_expr_.get(), &constraints_);
  }
}

SequentialScanNode::SequentialScanNode(
    const SequentialScanNode& other) :
    table_name_(other.table_name_),
    output_columns_(other.output_columns_),
    aggr_strategy_(other.aggr_strategy_),
    constraints_(other.constraints_) {
  for (const auto& e : other.select_list_) {
    select_list_.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  if (!other.where_expr_.isEmpty()) {
    where_expr_ = Some(
        other.where_expr_.get()->deepCopyAs<ValueExpressionNode>());
  }
}

Option<RefPtr<ValueExpressionNode>> SequentialScanNode::whereExpression() const {
  return where_expr_;
}

void SequentialScanNode::setWhereExpression(RefPtr<ValueExpressionNode> e) {
  where_expr_ = Some(e);
  constraints_.clear();
  QueryTreeUtil::findConstraints(e, &constraints_);
}

const String& SequentialScanNode::tableName() const {
  return table_name_;
}

const String& SequentialScanNode::tableAlias() const {
  return table_alias_;
}

void SequentialScanNode::setTableName(const String& table_name) {
  table_name_ = table_name;
}

void SequentialScanNode::setTableAlias(const String& table_alias) {
  table_alias_ = table_alias;
}

Vector<RefPtr<SelectListNode>> SequentialScanNode::selectList() const {
  return select_list_;
}

void SequentialScanNode::findSelectedColumnNames(
    RefPtr<ValueExpressionNode> expr,
    Set<String>* columns) const {
  auto colname = dynamic_cast<ColumnReferenceNode*>(expr.get());
  if (colname) {
    columns->emplace(normalizeColumnName(colname->fieldName()));
  }

  for (const auto& a : expr->arguments()) {
    findSelectedColumnNames(a, columns);
  }
}

Set<String> SequentialScanNode::selectedColumns() const {
  Set<String> columns;

  for (const auto& sl : select_list_) {
    findSelectedColumnNames(sl->expression(), &columns);
  }

  return columns;
}

Vector<String> SequentialScanNode::outputColumns() const {
  return output_columns_;
}

Vector<QualifiedColumn> SequentialScanNode::allColumns() const {
  String qualifier;
  if (table_alias_.empty()) {
    qualifier = table_name_ + ".";
  } else {
    qualifier = table_alias_ + ".";
  }

  Vector<QualifiedColumn> cols;
  for (const auto& c : table_columns_) {
    QualifiedColumn qc;
    qc.short_name = c;
    qc.qualified_name = qualifier + c;
    cols.emplace_back(qc);
  }

  return cols;
}

void SequentialScanNode::normalizeColumnNames() {
  auto normalizer = [this] (RefPtr<ColumnReferenceNode> expr) {
    expr->setColumnName(normalizeColumnName(expr->columnName()));
  };

  for (auto& sl : select_list_) {
    QueryTreeUtil::findColumns(sl->expression(), normalizer);
    output_columns_.emplace_back(sl->columnName());
  }

  if (!where_expr_.isEmpty()) {
    QueryTreeUtil::findColumns(where_expr_.get(), normalizer);
  }
}

String SequentialScanNode::normalizeColumnName(const String& column_name) const {
  if (!table_name_.empty() &&
      StringUtil::beginsWith(column_name, table_name_ + ".")) {
    return column_name.substr(table_name_.size() + 1);
  }

  if (!table_alias_.empty() &&
      StringUtil::beginsWith(column_name, table_alias_ + ".")) {
    return column_name.substr(table_alias_.size() + 1);
  }

  return column_name;
}

size_t SequentialScanNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  bool have_name = !table_name_.empty();
  bool have_alias = !table_alias_.empty();
  auto col = normalizeColumnName(column_name);
  auto col_with_name = table_name_ + "." + col;
  auto col_with_alias = table_alias_ + "." + col;

  for (size_t i = 0; i < select_list_.size(); ++i) {
    if ((select_list_[i]->columnName() == col) ||
        (have_name && select_list_[i]->columnName() == col_with_name) ||
        (have_alias && select_list_[i]->columnName() == col_with_alias)) {
      return i;
    }
  }

  bool found = false;
  for (const auto& c : table_columns_) {
    if (c == col) {
      found = true;
      break;
    }
  }

  if (!found) {
    return -1;
  }

  auto slnode = new SelectListNode(new ColumnReferenceNode(col));
  slnode->setAlias(col);
  select_list_.emplace_back(slnode);
  return select_list_.size() - 1;
}

AggregationStrategy SequentialScanNode::aggregationStrategy() const {
  return aggr_strategy_;
}

void SequentialScanNode::setAggregationStrategy(AggregationStrategy strategy) {
  aggr_strategy_ = strategy;
}

RefPtr<QueryTreeNode> SequentialScanNode::deepCopy() const {
  return new SequentialScanNode(*this);
}

String SequentialScanNode::toString() const {
  String aggr;
  switch (aggr_strategy_) {

    case AggregationStrategy::NO_AGGREGATION:
      aggr = "NO_AGGREGATION";
      break;

    case AggregationStrategy::AGGREGATE_WITHIN_RECORD_FLAT:
      aggr = "AGGREGATE_WITHIN_RECORD_FLAT";
      break;

    case AggregationStrategy::AGGREGATE_WITHIN_RECORD_DEEP:
      aggr = "AGGREGATE_WITHIN_RECORD_DEEP";
      break;

    case AggregationStrategy::AGGREGATE_ALL:
      aggr = "AGGREGATE_ALL";
      break;

  };

  auto str = StringUtil::format(
      "(seqscan (table $0) (aggregate $1) (select-list",
      table_name_,
      aggr);

  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }
  str += ")";

  if (!where_expr_.isEmpty()) {
    str += StringUtil::format(" (where $0)", where_expr_.get()->toString());
  }

  str += ")";
  return str;
}

const Vector<ScanConstraint>& SequentialScanNode::constraints() const {
  return constraints_;
}

} // namespace csql
