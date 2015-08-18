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

namespace csql {
namespace expressions {

void eqExpr(int argc, SValue* argv, SValue* out);
void neqExpr(int argc, SValue* argv, SValue* out);
void andExpr(int argc, SValue* argv, SValue* out);
void orExpr(int argc, SValue* argv, SValue* out);
void negExpr(int argc, SValue* argv, SValue* out);
void ltExpr(int argc, SValue* argv, SValue* out);
void lteExpr(int argc, SValue* argv, SValue* out);
void gtExpr(int argc, SValue* argv, SValue* out);
void gteExpr(int argc, SValue* argv, SValue* out);

}
}
#endif
