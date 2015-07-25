/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_SQLEXTENSIONS_AREACHARTBUILDER_H
#define _FNORDMETRIC_SQLEXTENSIONS_AREACHARTBUILDER_H
#include <chartsql/runtime/charts/chartbuilder.h>

namespace csql {
class DrawStatement;

class AreaChartBuilder : public ChartBuilder {
public:
  AreaChartBuilder(
      stx::chart::Canvas* canvas,
      RefPtr<DrawStatementNode> draw_stmt);
  stx::chart::Drawable* getChart() const override;
  std::string chartName() const override;
};

}
#endif
