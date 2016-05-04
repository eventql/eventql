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
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/runtime.h>
#include <eventql/sql/expressions/aggregate.h>
#include <eventql/sql/expressions/boolean.h>
#include <eventql/sql/expressions/conversion.h>
#include <eventql/sql/expressions/datetime.h>
#include <eventql/sql/expressions/internal.h>
#include <eventql/sql/expressions/math.h>
#include <eventql/sql/expressions/string.h>

using namespace stx;

namespace csql {

void installDefaultSymbols(SymbolTable* rt);

} // namespace csql
