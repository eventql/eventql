/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#pragma once
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/sql/qtree/ChartStatementNode.h>
#include <eventql/sql/qtree/DrawStatementNode.h>
#include <eventql/util/charts/canvas.h>

namespace csql {

class ChartExpression : public TableExpression {
public:
  ChartExpression(
      Transaction* txn,
      RefPtr<ChartStatementNode> qtree,
      Vector<Vector<ScopedPtr<TableExpression>>> input_tables,
      Vector<Vector<RefPtr<TableExpressionNode>>> input_table_qtrees);

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* nrecords) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

  bool next(SValue* row, size_t row_len) override;

protected:

  void executeDrawStatement(
      size_t idx,
      util::chart::Canvas* canvas);

  template <typename ChartBuilderType>
  util::chart::Drawable* executeDrawStatementWithChartType(
      size_t idx,
      util::chart::Canvas* canvas) {
    ChartBuilderType chart_builder(
        canvas,
        qtree_->getDrawStatements()[idx].asInstanceOf<DrawStatementNode>());

    for (size_t i = 0; i < input_tables_[idx].size(); ++i) {
      chart_builder.executeStatement(
          txn_,
          input_table_qtrees_[idx][i],
          input_tables_[idx][i].get());
    }

    return chart_builder.getChart();
  }

  void applyAxisDefinitions(
      RefPtr<DrawStatementNode> draw_stmt,
      util::chart::Drawable* chart) const;

  void applyAxisLabels(ASTNode* ast, util::chart::AxisDefinition* axis) const;

  void applyDomainDefinitions(
      RefPtr<DrawStatementNode> draw_stmt,
      util::chart::Drawable* chart) const;

  void applyGrid(
      RefPtr<DrawStatementNode> draw_stmt,
      util::chart::Drawable* chart) const;

  void applyLegend(
      RefPtr<DrawStatementNode> draw_stmt,
      util::chart::Drawable* chart) const;

  void applyTitle(
      RefPtr<DrawStatementNode> draw_stmt,
      util::chart::Drawable* chart) const;

  Transaction* txn_;
  Runtime* runtime_;
  RefPtr<ChartStatementNode> qtree_;
  Vector<Vector<ScopedPtr<TableExpression>>> input_tables_;
  Vector<Vector<RefPtr<TableExpressionNode>>> input_table_qtrees_;
  size_t counter_;
  String svg_data_;
};

} // namespace csql

