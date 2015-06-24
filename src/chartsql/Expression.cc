/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/Expression.h>

using namespace fnord;

namespace csql {

SumExpression::SumExpression() : value(0) {}

void SumExpression::result(SValue* out) {
  *out = SValue(SValue::FloatType(value));
}

void SumExpression::dumpTo(util::BinaryMessageWriter* data) {
  auto bytes = IEEE754::toBytes(value);
  data->appendUInt64(bytes);
}

void SumExpression::mergeFrom(util::BinaryMessageReader* data) {
  auto bytes = *data->readUInt64();
  value += IEEE754::fromBytes(bytes);
}

}
