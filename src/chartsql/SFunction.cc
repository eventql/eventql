/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/SFunction.h>

using namespace stx;

namespace csql {

PureFunction::PureFunction() : call(nullptr) {}

PureFunction::PureFunction(
    void (*_call)(int argc, SValue* in, SValue* out)) :
    call(_call) {}

SFunction::SFunction() :
    type(FN_PURE),
    vtable{ .t_pure = nullptr } {}

SFunction::SFunction(
    PureFunction fn) :
    type(FN_PURE),
    vtable{ .t_pure = fn } {}

SFunction::SFunction(
    AggregateFunction fn) :
    type(FN_AGGREGATE),
    vtable{ .t_aggregate = fn } {}

}
