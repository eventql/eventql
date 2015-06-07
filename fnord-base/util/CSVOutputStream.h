/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord-base/stdtypes.h>
#include <fnord-base/exception.h>
#include <fnord-base/io/outputstream.h>

namespace fnord {

class CSVOutputStream {
public:

  CSVOutputStream(
      std::unique_ptr<OutputStream> output_stream,
      char col_sep = ';',
      char row_sep = '\n');

  void appendRow(const Vector<String> row);

protected:
  std::shared_ptr<OutputStream> output_;
  char col_sep_;
  char row_sep_;
};

} // namespace fnord

