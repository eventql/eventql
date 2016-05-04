/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stringutil.h>
#include <stx/csv/BinaryCSVOutputStream.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

namespace stx {

BinaryCSVOutputStream::BinaryCSVOutputStream(
    std::unique_ptr<OutputStream> output_stream) :
    output_(std::move(output_stream)) {}

void BinaryCSVOutputStream::appendRow(const Vector<String>& row) {
  output_->appendVarUInt(row.size());

  for (int i = 0; i < row.size(); ++i) {
    output_->appendLenencString(row[i]);
  }
}

} // namespace stx

