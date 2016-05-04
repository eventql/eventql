/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <csql/svalue.h>
#include <csql/Transaction.h>

namespace csql {
namespace expressions {

void startsWithExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);

void endsWithExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);

void upperCaseExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);

void lowerCaseExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);

}
}
