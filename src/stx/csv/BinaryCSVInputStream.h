/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/io/inputstream.h"
#include "stx/csv/CSVInputStream.h"

namespace stx {

class BinaryCSVInputStream : public CSVInputStream {
public:

  /**
   * Create a new CSVInputStream from the provided InputStream.
   */
  explicit BinaryCSVInputStream(
      std::unique_ptr<RewindableInputStream>&& input_stream);

  /**
   * Read the next row from the csv file. Returns true if a row was read and
   * false on EOF. May raise an exception.
   */
  bool readNextRow(std::vector<std::string>* target) override;

  /**
   * Skip the next row from the csv file. Returns true if a row was skipped and
   * false on EOF. May raise an exception.
   */
  bool skipNextRow() override;

  /**
   * Rewind the input stream
   */
  void rewind() override;

  /**
   * Return the input stream
   */
  const RewindableInputStream& getInputStream() const override;

protected:
  std::unique_ptr<RewindableInputStream> input_;
};

}
