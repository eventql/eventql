/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/expressions/conversion.h>

namespace csql {
namespace expressions {

void toStringExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SQL_STRING) {
    *out = *argv;
  } else {
    *out = argv->toString();
  }
}

void toIntExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SQL_INTEGER) {
    *out = *argv;
  } else {
    *out = argv->toInteger();
  }
}

void toFloatExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SQL_FLOAT) {
    *out = *argv;
  } else {
    *out = argv->toFloat();
  }
}

void toBoolExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SQL_BOOL) {
    *out = *argv;
  } else {
    *out = argv->toBool();
  }
}

}
}
