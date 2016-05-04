/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/charts/ChartStatement.h>

namespace csql {

ChartStatement::ChartStatement(
    Vector<ScopedPtr<DrawStatement>> draw_statements) :
    draw_statements_(std::move(draw_statements)) {}

void ChartStatement::prepare(ExecutionContext* context) {
  for (auto& draw_stmt : draw_statements_) {
    draw_stmt->prepare(context);
  }
}

void ChartStatement::execute(
    ExecutionContext* context,
    stx::chart::RenderTarget* target) {

  stx::chart::Canvas canvas;
  for (auto& draw_stmt : draw_statements_) {
    draw_stmt->execute(context, &canvas);
  }

  canvas.render(target);
}

}

