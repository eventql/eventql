/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/io/outputstream.h>

namespace stx {

class BinaryCSVOutputStream {
public:

  BinaryCSVOutputStream(std::unique_ptr<OutputStream> output_stream);

  void appendRow(const Vector<String>& row);

protected:
  std::shared_ptr<OutputStream> output_;
};

} // namespace stx

