/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/sql/runtime/QueryBuilder.h>

#include "eventql/eventql.h"

namespace csql {

QueryBuilder::QueryBuilder(
    RefPtr<ValueExpressionBuilder> scalar_exp_builder) :
    scalar_exp_builder_(scalar_exp_builder) {}

ValueExpression QueryBuilder::buildValueExpression(
    Transaction* ctx,
    RefPtr<ValueExpressionNode> node) {
  return scalar_exp_builder_->compile(ctx, node);
}

//ScopedPtr<ChartStatement> QueryBuilder::buildChartStatement(
//    Transaction* ctx,
//    RefPtr<ChartStatementNode> node,
//    RefPtr<TableProvider> tables,
//    Runtime* runtime) {
//  Vector<ScopedPtr<DrawStatement>> draw_statements;
//
//  for (size_t i = 0; i < node->numChildren(); ++i) {
//    Vector<ScopedPtr<Task>> union_tables;
//
//    auto draw_stmt_node = node->child(i).asInstanceOf<DrawStatementNode>();
//    for (const auto& table : draw_stmt_node->inputTables()) {
//      //union_tables.emplace_back(
//      //    table_exp_builder_->build(ctx, table, this, tables.get()));
//    }
//
//    draw_statements.emplace_back(
//        new DrawStatement(ctx, draw_stmt_node, std::move(union_tables), runtime));
//  }
//
//  return mkScoped(new ChartStatement(std::move(draw_statements)));
//}

} // namespace csql
