/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_SQL_EXPRESSIONS_BOOLEAN_H
#define _FNORDMETRIC_SQL_EXPRESSIONS_BOOLEAN_H
#include <csql/svalue.h>
#include <csql/Transaction.h>

namespace csql {
namespace expressions {

void eqExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void neqExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void andExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void orExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void negExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void ltExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void lteExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void gtExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void gteExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void isNullExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);

}
}
#endif
