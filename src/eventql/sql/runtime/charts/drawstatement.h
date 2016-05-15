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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/sql/qtree/DrawStatementNode.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/Transaction.h>
#include <eventql/util/exception.h>
#include <eventql/util/autoref.h>
#include <cplot/canvas.h>
#include <cplot/drawable.h>

namespace csql {
class Runtime;

//class DrawStatement : public Statement {
//public:
//
//  DrawStatement(
//      Transaction* ctx,
//      RefPtr<DrawStatementNode> node,
//      Vector<ScopedPtr<Task>> sources,
//      Runtime* runtime);
//
//protected:
//
//  template <typename ChartBuilderType>
//  util::chart::Drawable* executeWithChart(
//      ExecutionContext* context,
//      util::chart::Canvas* canvas) {
//    ChartBuilderType chart_builder(canvas, node_);
//
//    for (auto& source : sources_) {
//      chart_builder.executeStatement(source.get(), context);
//    }
//
//    return chart_builder.getChart();
//  }
//
//  void applyAxisDefinitions(util::chart::Drawable* chart) const;
//  void applyAxisLabels(ASTNode* ast, util::chart::AxisDefinition* axis) const;
//  void applyDomainDefinitions(util::chart::Drawable* chart) const;
//  void applyGrid(util::chart::Drawable* chart) const;
//  void applyLegend(util::chart::Drawable* chart) const;
//  void applyTitle(util::chart::Drawable* chart) const;
//
//  Transaction* ctx_;
//  RefPtr<DrawStatementNode> node_;
//  Vector<ScopedPtr<Task>> sources_;
//  Runtime* runtime_;
//};

}
