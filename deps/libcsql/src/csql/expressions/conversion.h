/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <csql/svalue.h>
#include <csql/Transaction.h>

namespace csql {
namespace expressions {

void toStringExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void toIntExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void toFloatExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void toBoolExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);

}
}
