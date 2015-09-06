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
#include <stx/stdtypes.h>
#include <csql/runtime/runtime.h>
#include <csql/expressions/aggregate.h>
#include <csql/expressions/boolean.h>
#include <csql/expressions/datetime.h>
#include <csql/expressions/math.h>
#include <csql/expressions/internal.h>

using namespace stx;

namespace csql {

void installDefaultSymbols(SymbolTable* rt);

} // namespace csql
