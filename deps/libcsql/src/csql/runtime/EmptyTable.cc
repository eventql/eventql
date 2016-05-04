/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/EmptyTable.h>

namespace csql {

EmptyTable::EmptyTable(
    const Vector<String>& column_names) :
    column_names_(column_names) {}

void EmptyTable::prepare(ExecutionContext* context) {}

void EmptyTable::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {}

Vector<String> EmptyTable::columnNames() const {
  return column_names_;
}

size_t EmptyTable::numColumns() const {
  return column_names_.size();
}

}
