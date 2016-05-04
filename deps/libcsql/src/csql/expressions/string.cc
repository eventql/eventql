/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/expressions/string.h>
#include <stx/stringutil.h>

namespace csql {
namespace expressions {

static void checkArgs(const char* symbol, int argc, int argc_expected) {
  if (argc != argc_expected) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for %s. expected: %i, got: %i",
        symbol,
        argc_expected,
        argc);
  }
}

void startsWithExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("STARTSWITH", argc, 2);

  auto val = StringUtil::beginsWith(argv[0].getString(), argv[1].getString());
  *out = SValue(SValue::BoolType(val));
}

void endsWithExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("ENDSWITH", argc, 2);

  auto val = StringUtil::endsWith(argv[0].getString(), argv[1].getString());
  *out = SValue(SValue::BoolType(val));
}

void upperCaseExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("UPPERCASE", argc, 1);
  auto val = argv[0].getString();
  StringUtil::toUpper(&val);
  *out = SValue(val);
}

void lowerCaseExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("LOWERCASE", argc, 1);
  auto val = argv[0].getString();
  StringUtil::toLower(&val);
  *out = SValue(val);
}

}
}
