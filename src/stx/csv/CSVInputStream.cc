/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "stx/csv/CSVInputStream.h"
#include "stx/exception.h"
#include "stx/io/inputstream.h"

namespace stx {

std::unique_ptr<CSVInputStream> CSVInputStream::openFile(
    const std::string& file_path,
    char column_separator /* = ';' */,
    char row_separator /* = '\n' */,
    char quote_char /* = '"' */) {
  auto file = FileInputStream::openFile(file_path);
  file->readByteOrderMark();

  auto csv_file = new DefaultCSVInputStream(
      std::move(file),
      column_separator,
      row_separator,
      quote_char);

  return std::unique_ptr<CSVInputStream>(csv_file);
}

DefaultCSVInputStream::DefaultCSVInputStream(
    std::unique_ptr<RewindableInputStream>&& input_stream,
    char column_separator /* = ';' */,
    char row_separator /* = '\n' */,
    char quote_char /* = '"' */) :
    input_(std::move(input_stream)),
    column_separator_(column_separator),
    row_separator_(row_separator),
    quote_char_(quote_char) {}

// FIXPAUL quotechar unescaping...
bool DefaultCSVInputStream::readNextRow(std::vector<std::string>* target) {
  target->clear();

  bool eof = false;

  for (;;) {
    std::string column;
    char byte;
    bool quoted = false;

    for (;;) {
      if (!input_->readNextByte(&byte)) {
        eof = true;
        break;
      }

      if (!quoted && byte == column_separator_) {
        break;
      }

      if (!quoted && byte == row_separator_) {
        break;
      }

      if (byte == quote_char_) {
        quoted = !quoted;
      }

      column += byte;
    }

    target->emplace_back(column);

    if (eof || byte == row_separator_) {
      break;
    }
  }

  return !eof;
}

bool DefaultCSVInputStream::skipNextRow() {
  std::vector<std::string> devnull;
  return readNextRow(&devnull);
}

void DefaultCSVInputStream::rewind() {
  input_->rewind();
}

const RewindableInputStream& DefaultCSVInputStream::getInputStream() const {
  return *input_;
}


}
