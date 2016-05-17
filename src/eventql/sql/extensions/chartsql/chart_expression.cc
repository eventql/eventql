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
#include <eventql/sql/extensions/chartsql/chart_expression.h>
#include <eventql/sql/extensions/chartsql/linechartbuilder.h>
#include <eventql/sql/extensions/chartsql/areachartbuilder.h>
#include <eventql/sql/extensions/chartsql/barchartbuilder.h>
#include <eventql/sql/extensions/chartsql/pointchartbuilder.h>
#include <eventql/sql/extensions/chartsql/domainconfig.h>
#include <cplot/svgtarget.h>
#include <cplot/canvas.h>

namespace csql {

ChartExpression::ChartExpression(
    Transaction* txn,
    RefPtr<ChartStatementNode> qtree,
    Vector<Vector<ScopedPtr<TableExpression>>> input_tables,
    Vector<Vector<RefPtr<TableExpressionNode>>> input_table_qtrees) :
    txn_(txn),
    runtime_(txn->getRuntime()),
    qtree_(std::move(qtree)),
    input_tables_(std::move(input_tables)),
    input_table_qtrees_(input_table_qtrees),
    counter_(0) {}

ScopedPtr<ResultCursor> ChartExpression::execute() {
  util::chart::Canvas canvas;
  for (size_t i = 0; i < qtree_->getDrawStatements().size(); ++i) {
    executeDrawStatement(i, &canvas);
  }

  auto svg_data_os = StringOutputStream::fromString(&svg_data_);
  util::chart::SVGTarget svg(svg_data_os.get());
  canvas.render(&svg);

  return mkScoped(
      new DefaultResultCursor(
          1,
          std::bind(
              &ChartExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

void ChartExpression::executeDrawStatement(
    size_t idx,
    util::chart::Canvas* canvas) {
  auto draw_stmt = qtree_
      ->getDrawStatements()[idx]
      .asInstanceOf<DrawStatementNode>();

  util::chart::Drawable* chart = nullptr;

  switch (draw_stmt->chartType()) {
    case DrawStatementNode::ChartType::AREACHART:
      chart = executeDrawStatementWithChartType<AreaChartBuilder>(idx, canvas);
      break;
    case DrawStatementNode::ChartType::BARCHART:
      chart = executeDrawStatementWithChartType<BarChartBuilder>(idx, canvas);
      break;
    case DrawStatementNode::ChartType::LINECHART:
      chart = executeDrawStatementWithChartType<LineChartBuilder>(idx, canvas);
      break;
    case DrawStatementNode::ChartType::POINTCHART:
      chart = executeDrawStatementWithChartType<PointChartBuilder>(idx, canvas);
      break;
  }

  applyDomainDefinitions(draw_stmt, chart);
  applyTitle(draw_stmt, chart);
  applyAxisDefinitions(draw_stmt, chart);
  applyGrid(draw_stmt, chart);
  applyLegend(draw_stmt, chart); 
}

void ChartExpression::applyAxisDefinitions(
    RefPtr<DrawStatementNode> draw_stmt,
    util::chart::Drawable* chart) const {
  for (const auto& child : draw_stmt->ast()->getChildren()) {
    if (child->getType() != ASTNode::T_AXIS ||
        child->getChildren().size() < 1 ||
        child->getChildren()[0]->getType() != ASTNode::T_AXIS_POSITION) {
      continue;
    }

    util::chart::AxisDefinition* axis = nullptr;

    if (child->getChildren().size() < 1) {
      RAISE(kRuntimeError, "corrupt AST: AXIS has < 1 child");
    }

    switch (child->getChildren()[0]->getToken()->getType()) {
      case Token::T_TOP:
        axis = chart->addAxis(util::chart::AxisDefinition::TOP);
        break;

      case Token::T_RIGHT:
        axis = chart->addAxis(util::chart::AxisDefinition::RIGHT);
        break;

      case Token::T_BOTTOM:
        axis = chart->addAxis(util::chart::AxisDefinition::BOTTOM);
        break;

      case Token::T_LEFT:
        axis = chart->addAxis(util::chart::AxisDefinition::LEFT);
        break;

      default:
        RAISE(kRuntimeError, "corrupt AST: invalid axis position");
    }

    for (int i = 1; i < child->getChildren().size(); ++i) {
      auto prop = child->getChildren()[i];

      if (prop->getType() == ASTNode::T_PROPERTY &&
          prop->getToken() != nullptr &&
          *prop->getToken() == Token::T_TITLE &&
          prop->getChildren().size() == 1) {
        auto axis_title = runtime_->evaluateConstExpression(
            txn_,
            prop->getChildren()[0]);
        axis->setTitle(axis_title.getString());
        continue;
      }

      if (prop->getType() == ASTNode::T_AXIS_LABELS) {
        applyAxisLabels(prop, axis);
      }
    }
  }
}

void ChartExpression::applyAxisLabels(
    ASTNode* ast,
    util::chart::AxisDefinition* axis) const {
  for (const auto& prop : ast->getChildren()) {
    if (prop->getType() != ASTNode::T_PROPERTY ||
        prop->getToken() == nullptr) {
      continue;
    }

    switch (prop->getToken()->getType()) {
      case Token::T_INSIDE:
        axis->setLabelPosition(util::chart::AxisDefinition::LABELS_INSIDE);
        break;
      case Token::T_OUTSIDE:
        axis->setLabelPosition(util::chart::AxisDefinition::LABELS_OUTSIDE);
        break;
      case Token::T_OFF:
        axis->setLabelPosition(util::chart::AxisDefinition::LABELS_OFF);
        break;
      case Token::T_ROTATE: {
        if (prop->getChildren().size() != 1) {
          RAISE(kRuntimeError, "corrupt AST: ROTATE has no children");
        }

        auto rot = runtime_->evaluateConstExpression(
            txn_,
            prop->getChildren()[0]);
        axis->setLabelRotation(rot.getValue<double>());
        break;
      }
      default:
        RAISE(kRuntimeError, "corrupt AST: LABELS has invalid token");
    }
  }
}

void ChartExpression::applyDomainDefinitions(
    RefPtr<DrawStatementNode> draw_stmt,
    util::chart::Drawable* chart) const {
  for (const auto& child : draw_stmt->ast()->getChildren()) {
    bool invert = false;
    bool logarithmic = false;
    ASTNode* min_expr = nullptr;
    ASTNode* max_expr = nullptr;

    if (child->getType() != ASTNode::T_DOMAIN) {
      continue;
    }

    if (child->getToken() == nullptr) {
      RAISE(kRuntimeError, "corrupt AST: DOMAIN has no token");
    }

    util::chart::AnyDomain::kDimension dim;
    switch (child->getToken()->getType()) {
      case Token::T_XDOMAIN:
        dim = util::chart::AnyDomain::DIM_X;
        break;
      case Token::T_YDOMAIN:
        dim = util::chart::AnyDomain::DIM_Y;
        break;
      case Token::T_ZDOMAIN:
        dim = util::chart::AnyDomain::DIM_Z;
        break;
      default:
        RAISE(kRuntimeError, "corrupt AST: DOMAIN has invalid token");
    }

    for (const auto& domain_prop : child->getChildren()) {
      switch (domain_prop->getType()) {
        case ASTNode::T_DOMAIN_SCALE: {
          auto min_max_expr = domain_prop->getChildren();
          if (min_max_expr.size() != 2 ) {
            RAISE(kRuntimeError, "corrupt AST: invalid DOMAIN SCALE");
          }
          min_expr = min_max_expr[0];
          max_expr = min_max_expr[1];
          break;
        }

        case ASTNode::T_PROPERTY: {
          if (domain_prop->getToken() != nullptr) {
            switch (domain_prop->getToken()->getType()) {
              case Token::T_INVERT:
                invert = true;
                continue;
              case Token::T_LOGARITHMIC:
                logarithmic = true;
                continue;
              default:
                break;
            }
          }

          RAISE(kRuntimeError, "corrupt AST: invalid DOMAIN property");
          break;
        }

        default:
          RAISE(kRuntimeError, "corrupt AST: unexpected DOMAIN child");

      }
    }

    DomainConfig domain_config(chart, dim);
    domain_config.setInvert(invert);
    domain_config.setLogarithmic(logarithmic);
    if (min_expr != nullptr && max_expr != nullptr) {
      domain_config.setMin(runtime_->evaluateConstExpression(txn_, min_expr));
      domain_config.setMax(runtime_->evaluateConstExpression(txn_, max_expr));
    }
  }
}

void ChartExpression::applyTitle(
    RefPtr<DrawStatementNode> draw_stmt,
    util::chart::Drawable* chart) const {
  for (const auto& child : draw_stmt->ast()->getChildren()) {
    if (child->getType() != ASTNode::T_PROPERTY ||
        child->getToken() == nullptr || !(
        child->getToken()->getType() == Token::T_TITLE ||
        child->getToken()->getType() == Token::T_SUBTITLE)) {
      continue;
    }

    if (child->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST: [SUB]TITLE has != 1 child");
    }

    auto title_eval = runtime_->evaluateConstExpression(
        txn_,
        child->getChildren()[0]);
    auto title_str = title_eval.getString();

    switch (child->getToken()->getType()) {
      case Token::T_TITLE:
        chart->setTitle(title_str);
        break;
      case Token::T_SUBTITLE:
        chart->setSubtitle(title_str);
        break;
      default:
        break;
    }
  }
}

void ChartExpression::applyGrid(
    RefPtr<DrawStatementNode> draw_stmt,
    util::chart::Drawable* chart) const {
  ASTNode* grid = nullptr;

  for (const auto& child : draw_stmt->ast()->getChildren()) {
    if (child->getType() == ASTNode::T_GRID) {
      grid = child;
      break;
    }
  }

  if (!grid) {
    return;
  }

  bool horizontal = false;
  bool vertical = false;

  for (const auto& prop : grid->getChildren()) {
    if (prop->getType() == ASTNode::T_PROPERTY && prop->getToken() != nullptr) {
      switch (prop->getToken()->getType()) {
        case Token::T_HORIZONTAL:
          horizontal = true;
          break;
        case Token::T_VERTICAL:
          vertical = true;
          break;
        default:
          RAISE(kRuntimeError, "corrupt AST: invalid GRID property");
      }
    }
  }

  if (horizontal) {
    chart->addGrid(util::chart::GridDefinition::GRID_HORIZONTAL);
  }

  if (vertical) {
    chart->addGrid(util::chart::GridDefinition::GRID_VERTICAL);
  }
}

void ChartExpression::applyLegend(
    RefPtr<DrawStatementNode> draw_stmt,
    util::chart::Drawable* chart) const {
  ASTNode* legend = nullptr;

  for (const auto& child : draw_stmt->ast()->getChildren()) {
    if (child->getType() == ASTNode::T_LEGEND) {
      legend = child;
      break;
    }
  }

  if (!legend) {
    return;
  }


  util::chart::LegendDefinition::kVerticalPosition vert_pos =
      util::chart::LegendDefinition::LEGEND_BOTTOM;
  util::chart::LegendDefinition::kHorizontalPosition horiz_pos =
      util::chart::LegendDefinition::LEGEND_LEFT;
  util::chart::LegendDefinition::kPlacement placement =
      util::chart::LegendDefinition::LEGEND_OUTSIDE;
  std::string title;

  for (const auto& prop : legend->getChildren()) {
    if (prop->getType() == ASTNode::T_PROPERTY && prop->getToken() != nullptr) {
      switch (prop->getToken()->getType()) {
        case Token::T_TOP:
          vert_pos = util::chart::LegendDefinition::LEGEND_TOP;
          break;
        case Token::T_RIGHT:
          horiz_pos = util::chart::LegendDefinition::LEGEND_RIGHT;
          break;
        case Token::T_BOTTOM:
          vert_pos = util::chart::LegendDefinition::LEGEND_BOTTOM;
          break;
        case Token::T_LEFT:
          horiz_pos = util::chart::LegendDefinition::LEGEND_LEFT;
          break;
        case Token::T_INSIDE:
          placement = util::chart::LegendDefinition::LEGEND_INSIDE;
          break;
        case Token::T_OUTSIDE:
          placement = util::chart::LegendDefinition::LEGEND_OUTSIDE;
          break;
        case Token::T_TITLE: {
          if (prop->getChildren().size() != 1) {
            RAISE(kRuntimeError, "corrupt AST: TITLE has no children");
          }

          auto sval = runtime_->evaluateConstExpression(
              txn_,
              prop->getChildren()[0]);

          title = sval.getString();
          break;
        }
        default:
          RAISE(
              kRuntimeError,
              "corrupt AST: LEGEND has invalid property");
      }
    }
  }

  chart->addLegend(vert_pos, horiz_pos, placement, title);
}

size_t ChartExpression::getNumColumns() const {
  return 1;
}

void ChartExpression::setCompletionCallback(Function<void()> cb) {
  completion_callback_ = cb;
}

bool ChartExpression::next(SValue* row, size_t row_len) {
  if (++counter_ == 1) {
    if (row_len > 0) {
      *row = SValue::newString(svg_data_);
    }

    if (completion_callback_) {
      completion_callback_();
    }

    return true;
  } else {
    return false;
  }
}

} //namespace csql
