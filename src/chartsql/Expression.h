/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <fnord/ieee754.h>
#include <fnord/util/binarymessagereader.h>
#include <fnord/util/binarymessagewriter.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {

/**
 * A pure/stateless expression that returns a single return value
 */
struct PureExpression {
  virtual ~PureExpression() {}

  virtual void compute(int argc, const SValue* argv, SValue* out) = 0;

};

/**
 * An aggregate expression that returns a single return value
 */
struct AggregateExpression {
  virtual ~AggregateExpression() {}

  virtual void compute(int argc, const SValue* argv) = 0;

  virtual void result(SValue* out) = 0;

};

/**
 * An aggregate expression that returns a single return value and can be
 * computed commutatively
 */
struct CommutativeExpression : public AggregateExpression {
  virtual void dumpTo(util::BinaryMessageWriter* data) = 0;
  virtual void mergeFrom(util::BinaryMessageReader* data) = 0;
};


struct SumExpression : public CommutativeExpression {
  SumExpression();
  void result(SValue* out) override;
  void dumpTo(util::BinaryMessageWriter* data);
  void mergeFrom(util::BinaryMessageReader* data);
  double value;
};

}
