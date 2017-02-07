/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/exception.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/sql/backends/csv/CSVInputStream.h"

namespace csql {
namespace backends {
namespace csv {

std::unique_ptr<CSVInputStream> CSVInputStream::openFile(
    const std::string& file_path,
    char column_separator /* = ';' */,
    char row_separator /* = '\n' */,
    char quote_char /* = '"' */) {
  auto file = FileInputStream::openFile(file_path);
  file->readByteOrderMark();

  auto csv_file = new CSVInputStream(
      std::move(file),
      column_separator,
      row_separator,
      quote_char);

  return std::unique_ptr<CSVInputStream>(csv_file);
}

CSVInputStream::CSVInputStream(
    std::unique_ptr<RewindableInputStream>&& input_stream,
    char column_separator /* = ';' */,
    char row_separator /* = '\n' */,
    char quote_char /* = '"' */) :
    input_(std::move(input_stream)),
    column_separator_(column_separator),
    row_separator_(row_separator),
    quote_char_(quote_char) {}

// FIXPAUL quotechar unescaping...
bool CSVInputStream::readNextRow(std::vector<std::string>* target) {
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
        continue;
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

bool CSVInputStream::skipNextRow() {
  std::vector<std::string> devnull;
  return readNextRow(&devnull);
}

void CSVInputStream::rewind() {
  input_->rewind();
}

const RewindableInputStream& CSVInputStream::getInputStream() const {
  return *input_;
}

} // namespace csv
} // namespace backends
} // namespace csql
