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
#include <stx/csv/CSVOutputStream.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

namespace stx {

CSVOutputStream::CSVOutputStream(
    std::unique_ptr<OutputStream> output_stream,
    String col_sep /* = ';' */,
    String row_sep /* = '\n' */) :
    output_(std::move(output_stream)),
    col_sep_(col_sep),
    row_sep_(row_sep) {}

void CSVOutputStream::appendRow(const Vector<String>& row) {
  Buffer buf;

  for (int i = 0; i < row.size(); ++i) {
    if (i > 0) {
      buf.append(col_sep_);
    }

    buf.append(row[i].data(), row[i].size());
  }

  buf.append(row_sep_);
  output_->write(buf);
}

} // namespace stx

